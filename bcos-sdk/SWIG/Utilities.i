%{
#include "bcos-utilities/FixedBytes.h"
#include "bcos-utilities/Common.h"
#include <string_view>

using namespace bcos;

inline bcos::bytesConstRef byteArrayToBytesConstRef(const char* data, size_t length) {
    return {(const bcos::byte*)data, length};
}
inline bcos::bytesConstRef bytesToBytesConstRef(std::vector<uint8_t> const& input) {
    return {input.data(), input.size()};
}
inline bcos::bytes bytesConstRefToBytes(bcos::bytesConstRef input) {
    return bcos::bytes(input.begin(), input.end());
}
inline bcos::bytes h256ToBytes(h256 const& input) {
    return {input.data(), input.data() + input.size()};
}
inline h256 bytesToH256(bcos::bytes const& input) {
    return h256(input.data(), input.size());
}
inline std::string h256ToString(const bcos::h256& hash) {
    return hash.hex();
}

inline std::string u256ToString(const bcos::u256& value) {
    return value.str();
}

inline std::string stringViewToString(std::string_view view) {
    return std::string{view};
}
inline std::string_view stringToStringView(const std::string& string) {
    return std::string_view{string};
}
%}

%include <stdint.i>
%include <std_vector.i>
%include <std_string.i>

%template(bytes) std::vector<uint8_t>;
using bcos::byte = uint8_t;
using bcos::bytes = std::vector<bcos::byte>;

%apply (char *STRING, size_t LENGTH) { (const char* data, size_t length) }
inline bcos::bytesConstRef byteArrayToBytesConstRef(const char* data, size_t length);
inline bcos::bytesConstRef bytesToBytesConstRef(std::vector<uint8_t> const& input);
inline bcos::bytes bytesConstRefToBytes(bcos::bytesConstRef input);
inline bcos::bytes h256ToBytes(h256 const& input);
inline h256 bytesToH256(bcos::bytes const& input);
inline std::string h256ToString(const bcos::h256& hash);

inline std::string u256ToString(const bcos::u256& value);

inline std::string stringViewToString(std::string_view view);
inline std::string_view stringToStringView(const std::string& string);
