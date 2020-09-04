#include "Rbo/Server/TablesLock.hpp"

namespace Rbo::Server {

void TablesLock::initMetatable() {
    using sol::meta_function;

    metatable_[meta_function::index] =
            [this](const sol::table target, const sol::object key) -> sol::object
    {
        if (index_[target] == sol::nil)
            throw UnknownConstTable {};

        return index_[target][key];
    };

    metatable_[meta_function::new_index] = [](const sol::table, const sol::object, const sol::object) {
        throw IllegalChange {};
    };
}

void TablesLock::operator()(sol::table target) {
    if (metatable_.empty())
        initMetatable();

    if (index_[target] != sol::nil)
        throw AlreadyConstTable {};

    index_[target] = ctx_.create_table();
    for (const auto [key, value] : target) {
        index_[target][key] = value;
        target[key] = sol::nil;
    }

    target[sol::metatable_key] = metatable_;
}

} // namespace Rbo::Server
