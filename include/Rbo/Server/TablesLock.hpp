#ifndef TABLESLOCK_HPP
#define TABLESLOCK_HPP

#include <sol/sol.hpp>

namespace Rbo::Server {

struct IllegalChange : std::logic_error {
    IllegalChange() : std::logic_error { "Modifying const table" } {}
};

struct UnknownConstTable : std::logic_error {
    UnknownConstTable() : std::logic_error { "This const table is unknown" } {}
};

struct AlreadyConstTable : std::logic_error {
    AlreadyConstTable() : std::logic_error { "Locking already locked table" } {}
};

class TablesLock {
private:
    sol::state& ctx_;
    sol::table index_;
    sol::table metatable_;

    void initMetatable();

public:
    TablesLock(sol::state& ctx) : ctx_ { ctx }, index_ { ctx.create_table() }, metatable_ { ctx.create_table() } {}

    TablesLock(const TablesLock&) = delete;
    TablesLock& operator=(const TablesLock&) = delete;

    bool operator==(const TablesLock&) const = delete;

    void operator()(sol::table target_to_lock);
    sol::table get(const sol::table key) { return index_[key].get<sol::table>(); }
};

} // namespace Rbo::Server

#endif // TABLESLOCK_HPP
