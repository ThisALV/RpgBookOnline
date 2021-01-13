#include <Rbo/Server/InstructionsProvider.hpp>

#include <Rbo/Game.hpp>

namespace Rbo::Server {

namespace {

template<typename Container> sol::as_container_t<Container> luaContainer(Container& c) {
    return sol::as_container(c);
}

template<typename T> sol::as_container_t<std::vector<T>> luaVector(std::vector<T>& v) {
    return luaContainer<std::vector<T>>(v);
}

template<typename K, typename V> sol::as_container_t<std::unordered_map<K, V>> luaUnorderedMap(std::unordered_map<K, V>& m) {
    return luaContainer<std::unordered_map<K, V>>(m);
}

}

void InstructionsProvider::initContainersAPI() {
    ctx_.new_usertype<std::vector<std::string>>("StringVector", sol::constructors<std::vector<std::string>()>(), "iterable", luaVector<std::string>);
    ctx_.new_usertype<std::vector<byte>>("ByteVector", sol::constructors<std::vector<byte>()>(), "iterable", luaVector<byte>);

    ctx_.new_usertype<std::unordered_map<byte, std::string>>("ByteWithString", sol::constructors<std::unordered_map<byte, std::string>()>(), "iterable", luaUnorderedMap<byte, std::string>);
    ctx_.new_usertype<std::unordered_map<std::string, std::string>>("StringWithString", sol::constructors<std::unordered_map<std::string, std::string>()>(), "iterable", luaUnorderedMap<std::string, std::string>);
    ctx_.new_usertype<std::unordered_map<byte, byte>>("ByteWithByte", sol::constructors<std::unordered_map<byte, byte>()>(), "iterable", luaUnorderedMap<byte, byte>);
    ctx_.new_usertype<Effects>("Effects", sol::constructors<Effects()>(), "iterable", luaContainer<Effects>);
}

} // namespace Rbo::Server
