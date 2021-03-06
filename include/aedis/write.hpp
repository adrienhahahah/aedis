/* Copyright (c) 2019 - 2021 Marcelo Zimbres Silva (mzimbres at gmail dot com)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <queue>
#include <chrono>

#include "config.hpp"
#include "request.hpp"

#include <boost/beast/core/stream_traits.hpp>

namespace aedis {

template<
   class SyncWriteStream,
   class Event>
std::size_t
write(
   SyncWriteStream& stream,
   request<Event>& req,
   boost::system::error_code& ec)
{
    static_assert(boost::beast::is_sync_write_stream<SyncWriteStream>::value,
       "SyncWriteStream type requirements not met");

    return write(stream, net::buffer(req.payload), ec);
}

template<
   class SyncWriteStream,
   class Event>
std::size_t
write(
   SyncWriteStream& stream,
   request<Event>& req)
{
    static_assert(boost::beast::is_sync_write_stream<SyncWriteStream>::value,
        "SyncWriteStream type requirements not met");

    boost::system::error_code ec;
    auto const bytes_transferred = write(stream, req, ec);

    if (ec)
        BOOST_THROW_EXCEPTION(boost::system::system_error{ec});

    return bytes_transferred;
}

template<
   class AsyncWriteStream,
   class Event,
   class CompletionToken =
      net::default_completion_token_t<typename AsyncWriteStream::executor_type>>
auto
async_write(
   AsyncWriteStream& stream,
   request<Event>& req,
   CompletionToken&& token =
      net::default_completion_token_t<typename AsyncWriteStream::executor_type>{})
{
   static_assert(boost::beast::is_async_write_stream<
      AsyncWriteStream>::value,
      "AsyncWriteStream type requirements not met");

   return net::async_write(stream, net::buffer(req.payload), token);
}

template <
  class AsyncReadStream,
  class Event>
struct writer_op {
   AsyncReadStream& stream;
   net::steady_timer& st;
   std::queue<request<Event>>* reqs;

   template <class Self>
   void operator()(
      Self& self,
      boost::system::error_code ec = {},
      std::size_t bytes_transferred = 0)
   {
      // To stop the operation users are required to close the socket
      // and cancel the timer.
      if (!stream.is_open()) {
         self.complete({});
         return;
      }

      // We don't leave on operation aborted as that is the error we
      // get when users cancel the timer to signal there is message to
      // be written.  This relies on the fact that async_write does
      // not complete with this error.
      if (ec && ec != net::error::operation_aborted) {
         self.complete(ec);
         return;
      }

      // When the condition below holds we are coming from a
      // successful write and should wait for the next write trigger.
      if (bytes_transferred != 0) {
	 st.expires_after(std::chrono::years{10});
	 st.async_wait(std::move(self));
	 return;
      }

      if (!std::empty(*reqs)) {
         async_write(
            stream,
            net::buffer(reqs->front().payload),
            std::move(self));
         return;
      }
   }
};

template <
   class AsyncWriteStream,
   class Event,
   class CompletionToken =
      net::default_completion_token_t<typename AsyncWriteStream::executor_type>
   >
auto async_writer(
   AsyncWriteStream& stream,
   std::queue<request<Event>>& reqs,
   net::steady_timer& writeTrigger,
   CompletionToken&& token =
      net::default_completion_token_t<typename AsyncWriteStream::executor_type>{})
{
   return net::async_compose
      < CompletionToken
      , void(boost::system::error_code)
      >(writer_op<AsyncWriteStream, Event> {stream, writeTrigger, &reqs},
        token,
        stream,
	writeTrigger);
}

// Returns true if a write has been triggered.
template <class Event, class Filler>
bool queue_writer(
   std::queue<request<Event>>& reqs,
   Filler filler,
   net::steady_timer& st)
{
   auto const empty = std::empty(reqs);
   if (empty || std::size(reqs) == 1)
      reqs.push({});

   filler(reqs.back());

   if (empty)
      st.cancel();

   return empty;
}
} // aedis

