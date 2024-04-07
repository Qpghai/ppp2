#include <ppp/net/asio/websocket.h>
#include <ppp/net/asio/templates/SslSocket.h>
#include <ppp/net/asio/templates/WebSocket.h>
#include <ppp/net/IPEndPoint.h>
#include <ppp/net/Ipep.h>
#include <ppp/net/Socket.h>
#include <ppp/coroutines/asio/asio.h>

namespace ppp {
    namespace net {
        namespace asio {
            bool websocket::Write(const void* buffer, int offset, int length, const AsynchronousWriteCallback& cb) noexcept {
                if (NULL == buffer || offset < 0 || length < 1) {
                    return false;
                }

                if (NULL == cb) {
                    return false;
                }

                if (IsDisposed() || !websocket_.is_open()) {
                    return false;
                }

                const std::shared_ptr<websocket> self = shared_from_this();
                auto complete_do_write_async_callback = [self, this, cb, buffer, offset, length]() noexcept {
                    websocket_.async_write(boost::asio::buffer(((Byte*)buffer) + (offset), length),
                        [self, this, cb](const boost::system::error_code& ec, size_t sz) noexcept {
                            bool ok = ec == boost::system::errc::success;
                            if (cb) {
                                cb(ok); /* b is boost::system::errc::success. */
                            }
                        });
                    };

                return ppp::threading::Executors::Dispatch(context_, strand_, complete_do_write_async_callback);
            }
        }
    }
}