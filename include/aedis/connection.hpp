/* Copyright (c) 2019 - 2021 Marcelo Zimbres Silva (mzimbres at gmail dot com)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <memory>
#include <string>

#include "config.hpp"
#include "type.hpp"
#include "request.hpp"
#include "response_buffers.hpp"

namespace aedis {

template <class Event>
class connection :
   public std::enable_shared_from_this<connection<Event>> {
private:
   net::steady_timer timer_;
   net::ip::tcp::socket socket_;
   std::string buffer_;
   resp::response_buffers resps_;
   std::queue<request<Event>> reqs_;
   bool reconnect_ = true;

   void reset()
   {
      socket_.close();
      timer_.cancel();
      reqs_.push({});
      reqs_.front().hello();
   }

   template <class Receiver>
   net::awaitable<void>
   worker_coro(
      Receiver& recv,
      typename boost::asio::ip::tcp::resolver::results_type const& results)
   {
      auto ex = co_await net::this_coro::executor;

      net::steady_timer timer {ex};
      std::chrono::seconds wait_interval {1};

      boost::system::error_code ec;
      while (reconnect_) {
	 co_await async_connect(
	    socket_,
	    results,
	    net::redirect_error(net::use_awaitable, ec));

	 if (ec) {
	    reset();
	    recv.on_error(ec);
	    timer.expires_after(wait_interval);
	    co_await timer.async_wait(net::use_awaitable);
	    continue;
	 }

	 async_writer(socket_, reqs_, timer_, net::detached);

	 ec = {};
	 co_await co_spawn(
	    ex,
	    async_reader(socket_, buffer_, resps_, recv, reqs_, ec),
	    net::use_awaitable);

	 if (ec) {
	    reset();
	    recv.on_error(ec);
	    timer.expires_after(wait_interval);
	    co_await timer.async_wait(net::use_awaitable);
	    continue;
	 }
      }
   }

public:
   using event_type = Event;

   connection(net::io_context& ioc)
   : timer_{ioc}
   , socket_{ioc}
   {
      reqs_.push({});
      reqs_.front().hello();
   }

   template <class Receiver>
   void
   start(
      Receiver& recv,
      typename boost::asio::ip::tcp::resolver::results_type const& results)
   {
      auto self = this->shared_from_this();
      net::co_spawn(
	 socket_.get_executor(),
         [self, &recv, &results] () mutable { return self->worker_coro(recv, results); },
         net::detached);
   }

   template <class Filler>
   void send(Filler filler)
      { queue_writer(reqs_, filler, timer_); }

   void disable_reconnect()
   {
      reconnect_ = false;
   }
};

} // aedis
