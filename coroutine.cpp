#include <boost/asio.hpp>

#include "aedis.hpp"

namespace net = aedis::net;
using tcp = net::ip::tcp;
using tcp_socket = net::use_awaitable_t<>::as_default_on_t<tcp::socket>;

namespace this_coro = net::this_coro;

using namespace net;
using namespace aedis;

awaitable<void> example1(tcp::resolver::results_type const& r)
{
   tcp_socket socket {co_await this_coro::executor};

   co_await async_connect(socket, r);

   auto cmd = ping();
   co_await async_write(socket, buffer(cmd));

   resp::buffer buffer;
   co_await resp::async_read(socket, &buffer);

   resp::print(buffer.res);
}

awaitable<void> example2(tcp::resolver::results_type const& r)
{
   tcp_socket socket {co_await this_coro::executor};

   co_await async_connect(socket, r);

   auto cmd = multi()
            + ping()
            + incr("age")
            + exec()
	    + quit()
	    ;

   co_await async_write(socket, buffer(cmd));

   resp::buffer buffer;
   for (;;) {
      co_await resp::async_read(socket, &buffer);
      resp::print(buffer.res);
      buffer.res.clear();
   }
}

awaitable<void> example3(tcp::resolver::results_type const& r)
{
   tcp_socket socket {co_await this_coro::executor};

   co_await async_connect(socket, r);

   std::list<std::string> a
   {"one" ,"two", "three"};

   std::set<std::string> b
   {"a" ,"b", "c"};

   std::map<std::string, std::string> c
   { {{"Name"},      {"Marcelo"}} 
   , {{"Education"}, {"Physics"}}
   , {{"Job"},       {"Programmer"}}
   };

   std::map<int, std::string> d
   { {1, {"foo"}} 
   , {2, {"bar"}}
   , {3, {"foobar"}}
   };

   auto cmd = ping()
            + role()
            + flushall()
            + rpush("a", a)
            + lrange("a")
            + del("a")
            + multi()
            + rpush("b", b)
            + lrange("b")
            + del("b")
            + hset("c", c)
            + hincrby("c", "Age", 40)
            + hmget("c", {"Name", "Education", "Job"})
            + hvals("c")
            + hlen("c")
            + hgetall("c")
            + zadd({"d"}, d)
            + zrange("d")
            + zrangebyscore("foo", 2, -1)
            + set("f", {"39"})
            + incr("f")
            + get("f")
            + expire("f", 10)
            + publish("g", "A message")
            + exec()
	    + set("h", {"h"})
	    + append("h", "h")
	    + get("h")
	    + auth("password")
	    + bitcount("h")
	    + quit()
	    ;

   co_await async_write(socket, buffer(cmd));

   resp::buffer buffer;
   for (;;) {
      co_await resp::async_read(socket, &buffer);
      resp::print(buffer.res);
      buffer.res.clear();
   }
}

int main()
{
   io_context ioc {1};
   tcp::resolver res(ioc.get_executor());
   auto const r = res.resolve("127.0.0.1", "6379");

   co_spawn(ioc, example1(r), detached);
   co_spawn(ioc, example2(r), detached);
   co_spawn(ioc, example3(r), detached);

   ioc.run();
}