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
            static bool websocket_async_close(std::shared_ptr<websocket> ws, websocket::AsioWebSocket& websocket) noexcept {
                websocket.async_close(boost::beast::websocket::close_code::normal,
                    [ws, &websocket](const boost::system::error_code& ec_) noexcept {
                        Socket::Closesocket(websocket.next_layer());
                    });
                return true;
            }

            void websocket::Dispose() noexcept {
                auto self = shared_from_this();
                ppp::threading::Executors::Post(context_, strand_,
                    [self, this]() noexcept {
                        disposed_ = true;
                        websocket_async_close(self, websocket_);
                    });
            }

            bool websocket::ShiftToScheduler() noexcept {
                std::shared_ptr<boost::asio::ip::tcp::socket> socket_new;
                ppp::threading::Executors::ContextPtr scheduler;
                ppp::threading::Executors::StrandPtr strand;

                auto& socket = websocket_.next_layer();
                bool ok = ppp::threading::Executors::ShiftToScheduler(socket, socket_new, scheduler, strand);
                if (ok) {
                    socket = std::move(*socket_new);
                    strand_ = strand;
                    context_ = scheduler;
                }

                return ok;
            }
        }
    }
}