#include "sdk.h"

#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <iostream>
#include <thread>

#include "json_loader.h"
#include "http_handler/request_handler.h"
#include "http_server.h"

#include "command_line.h"
#include "ticker.h"

namespace sys = boost::system;
using namespace std::literals;
namespace net = boost::asio;

namespace {

// Запускает функцию fn на n потоках, включая текущий
template <typename Fn>
void RunWorkers(unsigned n, const Fn& fn) {
    n = std::max(1u, n);
    std::vector<std::jthread> workers;
    workers.reserve(n - 1);
    // Запускаем n-1 рабочих потоков, выполняющих функцию fn
    while (--n) {
        workers.emplace_back(fn);
    }
    fn();
}

}  // namespace

int main(int argc, const char* argv[]) {

    Args command_line_args;
    try {
        if (auto args = ParseCommandLine(argc, argv)) {
            command_line_args = *args;
        } else {
            return EXIT_FAILURE;
        }
    } catch (const std::exception& ex) {
        std::cout << "Failed parsing command line arguments" << std::endl;
        return EXIT_FAILURE;
    }

    try {
        // 1. Загружаем карту из файла и построить модель игры
        Logger logger;

        // 2. Инициализируем io_context
        const unsigned num_threads = std::thread::hardware_concurrency();
        net::io_context ioc(num_threads);

        auto api_strand = net::make_strand(ioc);

        std::filesystem::path config = std::filesystem::weakly_canonical(std::filesystem::path((command_line_args.config_file_path)));
        std::filesystem::path root = std::filesystem::weakly_canonical(std::filesystem::path((command_line_args.root_path)));
        unsigned int tick_period = command_line_args.tick_period;
        bool random_spawn = command_line_args.random_spawn;

        GameServer game_server(config);

        if (tick_period) {
            std::chrono::milliseconds tick_period_millisec(tick_period);
            auto ticker = std::make_shared<Ticker>(api_strand, tick_period_millisec, [&game_server](std::chrono::milliseconds delta) 
                                                                                                    {game_server.Tick(delta);});
            game_server.SetAutoTick();
            ticker->Start();
        }

        if (random_spawn) {
            game_server.SetRandSpawn();
        }

        // 3. Добавляем асинхронный обработчик сигналов SIGINT и SIGTERM
        net::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&ioc](const sys::error_code ec, [[maybe_unused]] int signal_number) {
            if (!ec) {
                ioc.stop();
                Logger::LogServerExit();
            }
        });

        // 4. Создаём обработчик HTTP-запросов и связываем его с моделью игры
        auto handler = std::make_shared<http_handler::RequestHandler>(root, api_strand, game_server);
        http_handler::LoggingRequestHandler<http_handler::RequestHandler> logging_handler{*handler};

        // 5. Запустить обработчик HTTP-запросов, делегируя их обработчику запросов
        const auto address = net::ip::make_address("0.0.0.0");
        constexpr net::ip::port_type port = 8080;

        Logger::LogServerStart(port, address.to_string());
        http_server::ServeHttp(ioc, {address, port}, logging_handler);

        // 6. Запускаем обработку асинхронных операций
        RunWorkers(std::max(1u, num_threads), [&ioc] {
            ioc.run();
        });

    } catch (const std::exception& ex) {
        Logger::LogServerExitBecauseErr(ex);
        return EXIT_FAILURE;
    }
}
