#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "nlohmann/json.hpp"

#include "schedule.h"

using json = nlohmann::json;

TEST_CASE("Correct schedule testing", "[schedule]") {
    const std::filesystem::path testfiles[] = {"../tests/test_schedules/basic_test_schedule.json"};
    auto sched = schedule::create_from_file(testfiles[0]);
    json schedule_description = json::parse(std::ifstream(testfiles[0]));

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

    auto sched_inst2(sched_inst);

    REQUIRE(events.size() > 0);
    REQUIRE(events[0].id() == schedule_description["schedule"]["events"][0]["id"].get<std::string>());
    REQUIRE(events[0].day() == days(schedule_description["schedule"]["events"][0]["day"].get<unsigned int>()));
    REQUIRE(events[0].assigned_actions()[0] ==
            schedule_description["schedule"]["events"][0]["actions"][0].get<std::string>());

    schedule_description["schedule"]["end_at"] = "10.01.1970";

    auto sched_with_end = schedule::create_from_description(schedule_description["schedule"]);

    REQUIRE(sched_with_end.has_value());

    auto sched_with_end_inst = *sched_with_end;

    REQUIRE(sched_with_end_inst.title() == schedule_description["schedule"]["title"].get<std::string>());
    REQUIRE(sched_with_end_inst.end_at().has_value());
    REQUIRE(sched_with_end_inst.end_at()->count() == 8);

    schedule_description["schedule"]["end_at"] = "01.01.1970";

    auto sched_with_start_eq_end = schedule::create_from_description(schedule_description["schedule"]);

    REQUIRE(sched_with_start_eq_end.has_value() == false);
}
