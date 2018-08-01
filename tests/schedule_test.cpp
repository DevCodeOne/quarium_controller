#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "nlohmann/json.hpp"

#include "schedule.h"

using json = nlohmann::json;

TEST_CASE("Schedule testing", "[schedule]") {
    const std::filesystem::path path_to_testfile = "../tests/test_schedules/basic_test_schedule.json";
    auto sched = schedule::create_from_file(path_to_testfile);
    json schedule_description = json::parse(std::ifstream(path_to_testfile));

    REQUIRE(sched.has_value());

    auto sched_inst = *sched;

    REQUIRE(sched_inst.period().count() == schedule_description["schedule"]["period_in_days"].get<unsigned int>());
    REQUIRE(sched_inst.start_at().has_value());
    REQUIRE(sched_inst.end_at().has_value() == false);
    REQUIRE(sched_inst.title() == schedule_description["schedule"]["title"].get<std::string>());
    REQUIRE(sched_inst.schedule_mode() == (schedule_description["schedule"]["repeating"].get<bool>()
                                               ? schedule::mode::repeating
                                               : schedule::mode::single_iteration));

    auto events = sched_inst.events();

    REQUIRE(events[0].id() == schedule_description["schedule"]["events"][0]["id"].get<std::string>());
    REQUIRE(events[0].day() == days(schedule_description["schedule"]["events"][0]["day"].get<unsigned int>()));
    REQUIRE(events[0].assigned_actions()[0] ==
            schedule_description["schedule"]["events"][0]["actions"][0].get<std::string>());
}
