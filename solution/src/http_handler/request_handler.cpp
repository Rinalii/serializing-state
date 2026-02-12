#include "request_handler.h"

namespace http_handler {

const std::unordered_map<std::string, std::string_view> extension_to_content_type = {
    {".html", content_type::HTML},
    {".css", content_type::CSS},
    {".txt", content_type::TXT},
    {".js", content_type::JS},
    {".json", content_type::JSON},
    {".xml", content_type::XML},
    {".png", content_type::PNG},
    {".jpg", content_type::JPG},
    {".jpe", content_type::JPG},
    {".jpeg", content_type::JPG},
    {".gif", content_type::GIF},
    {".bmp", content_type::BMP},
    {".ico", content_type::ICO},
    {".tiff", content_type::TIFF},
    {".tif", content_type::TIFF},
    {".svg", content_type::SVG},
    {".svgz", content_type::SVG},
    {".mp3", content_type::MP3}
};

using FileResponse = http::response<http::file_body>;
FileResponse MakeFileResponse(http::status status, fs::path path, unsigned http_version,
                                bool keep_alive, std::string_view content_type, std::string_view cache_control) {
    FileResponse response(status, http_version);
    response.set(http::field::content_type, content_type);
    http::file_body::value_type file;

    if (boost::system::error_code ec; file.open(path.c_str(), beast::file_mode::read, ec), ec) {
        std::cout << "Failed to open file "sv << path.c_str() << std::endl;
        throw std::logic_error(std::string("Failed to open file ") + path.c_str());
    }
    response.body() = std::move(file);
    response.keep_alive(keep_alive);
    response.prepare_payload();
    if (!cache_control.empty()) {
        response.set(http::field::cache_control, cache_control);
    }
    return response;
}

std::string UrlDecode(const std::string& str) {
    std::ostringstream decoded;
    std::istringstream encoded(str);
    encoded >> std::noskipws;

    char curr;
    while (encoded >> curr) {
        if (curr == '%') {
            int hex_code;
            if (!(encoded >> std::hex >> hex_code)) {
                decoded << '%';
            } else {
                decoded << static_cast<char>(hex_code);
            }
        } else if (curr == '+') {
            decoded << ' ';
        } else {
            decoded << curr;
        }
    }
    return decoded.str();
}

std::string_view GetContentType(fs::path path) {
    std::string extension = path.extension().string();

    auto it = extension_to_content_type.find(extension);
    if(it != extension_to_content_type.end()) {
        return it->second;
    }
    return content_type::UNKNOWN;
}

bool IsSubPath(fs::path path, fs::path base) {
    // Приводим оба пути к каноничному виду (без . и ..)
    path = fs::weakly_canonical(path);
    base = fs::weakly_canonical(base);

    // Проверяем, что все компоненты base содержатся внутри path
    for (auto b = base.begin(), p = path.begin(); b != base.end(); ++b, ++p) {
        if (p == path.end() || *p != *b) {
            return false;
        }
    }
    return true;
}

RequestType DetermineRequestType(const std::string& target_str) {
    if (target_str.find("/api/") == 0) {
        return RequestType::API;
    } else {
        return RequestType::STATIC;
    }
}

}
