#include <boost/log/trivial.hpp>     // для BOOST_LOG_TRIVIAL
#include <boost/log/core.hpp>        // для logging::core
#include <boost/log/expressions.hpp> // для выражения, задающего фильтр

#include <boost/log/utility/setup/file.hpp>                 //for add_file_log
#include <boost/log/utility/setup/common_attributes.hpp>    //for timestamp
#include <boost/log/utility/setup/console.hpp>              //for add_console_log

#include <boost/date_time.hpp>      //вывод момента времени
#include <boost/log/utility/manipulators/add_value.hpp>     //for logging::add_value

#include <boost/json.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <fstream>

#include <string_view>

using namespace std::literals;

namespace logging = boost::log;
namespace sinks = boost::log::sinks;
namespace keywords = boost::log::keywords;


BOOST_LOG_ATTRIBUTE_KEYWORD(additional_data, "AdditionalData", boost::json::value)
BOOST_LOG_ATTRIBUTE_KEYWORD(timestamp, "TimeStamp", boost::posix_time::ptime)

class Logger {
public:
    Logger() {
        logging::add_common_attributes();
        logging::add_console_log(std::cout,
            keywords::format = &MyFormatter,
            keywords::auto_flush = true
        );
    }

    static void LogServerStart(int port, std::string address) {
        boost::json::object add_data;
        add_data["port"] = port;
        add_data["address"] = address;
        BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, add_data) << "server started";
    }

    static void LogServerExit() {
        boost::json::object add_data;
        add_data["code"] = 0;
        BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, add_data) << "server exited";
    }

    static void LogServerExitBecauseErr(const std::exception& ex) {
        boost::json::object add_data;
        add_data["code"] = 1;
        add_data["exception"] = ex.what();
        BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, add_data) << "server exited";
    }

private:
    typedef sinks::synchronous_sink< sinks::text_file_backend > sink_t;
    boost::shared_ptr<sink_t> g_file_sink;

    static void MyFormatter (logging::record_view const& rec, logging::formatting_ostream& strm) {
        auto ts = *rec[timestamp];
        auto data = *rec[additional_data];
        auto message = rec[logging::expressions::message];

        boost::json::object log_message;
        
        log_message["timestamp"] = to_iso_extended_string(ts);
        
        boost::json::object data_obj;
        if (data.is_object()) {
            for (const auto& pair : data.as_object()) {
                data_obj[pair.key()] = pair.value();
            }
        }
        log_message["data"] = data_obj;
        log_message["message"] = message.get<std::string>();
        strm << boost::json::serialize(log_message);
    } 

};