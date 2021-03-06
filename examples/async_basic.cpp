/* Copyright (c) 2019 - 2021 Marcelo Zimbres Silva (mzimbres at gmail dot com)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include <aedis/aedis.hpp>

using namespace aedis;

/* This example shows how to receive and send events.
 *
 * 1. Create a connection obeject.
 *
 * 2. Start sending commands after the hello command has been
 *    received.
 *
 * As a rule, every redis command is received in a function named
 * on_command. The user has to override the base class version to
 * start receiving events.
 */

enum class events {one, two, three, ignore};

class receiver : public receiver_base<events> {
private:
   std::shared_ptr<connection<events>> conn_;

public:
   using event_type = events;
   receiver(std::shared_ptr<connection<events>> conn) : conn_{conn} { }

   void on_hello(events ev, resp::array_type& v) noexcept override
   {
      auto f = [](auto& req)
      {
	 req.ping(events::one);
	 req.quit();
      };

      conn_->disable_reconnect();
      conn_->send(f);
   }

   void on_ping(events ev, resp::simple_string_type& s) noexcept override
      { std::cout << "PING: " << s << std::endl; }

   void on_quit(events ev, resp::simple_string_type& s) noexcept override
      { std::cout << "QUIT: " << s << std::endl; }
};

int main()
{
   net::io_context ioc {1};
   net::ip::tcp::resolver resolver{ioc};
   auto const results = resolver.resolve("127.0.0.1", "6379");
   auto conn = std::make_shared<connection<events>>(ioc);
   receiver recv{conn};
   conn->start(recv, results);
   ioc.run();
}



