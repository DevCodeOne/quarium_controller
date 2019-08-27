#define CATCH_CONFIG_MAIN
#include "catch2/catch.hpp"

#include <iostream>

#include "nlohmann/json.hpp"

#include "schedule/schedule_handler.h"

using json = nlohmann::json;

TEST_CASE("Correct schedule testing") {
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
    REQUIRE(events.begin()->name() == schedule_description["schedule"]["events"][0]["name"].get<std::string>());
    REQUIRE(events.begin()->day() == days(schedule_description["schedule"]["events"][0]["day"].get<unsigned int>()));
    REQUIRE(*events.begin()->actions().begin() ==
            schedule_description["schedule"]["events"][0]["actions"][0].get<std::string>());

    schedule_description["schedule"]["period_in_days"] = 1u;
    schedule_description["schedule"]["end_at"] = "10.01.1970";

    auto sched_with_end = schedule::deserialize(schedule_description["schedule"]);

    REQUIRE(sched_with_end.has_value());

    auto sched_with_end_inst = *sched_with_end;

    REQUIRE(sched_with_end_inst.title() == schedule_description["schedule"]["title"].get<std::string>());
    REQUIRE(sched_with_end_inst.end_at().has_value());
    REQUIRE(sched_with_end_inst.end_at()->count() == 9);

    schedule_description["schedule"]["end_at"] = "01.01.1970";

    auto sched_with_start_eq_end = schedule::deserialize(schedule_description["schedule"]);

    REQUIRE(sched_with_start_eq_end.has_value() == false);

    schedule_description["schedule"]["end_at"] = "13.01.1970";
    schedule_description["schedule"]["events"].push_back(R"({
                "name" : "light_off",
                "day" : 3,
                "trigger_at" : "12:00",
                "actions" : ["light_off"]
            })"_json);

    auto sched_with_calc_period = schedule::deserialize(schedule_description["schedule"]);

    REQUIRE(sched_with_calc_period.has_value());
    REQUIRE(sched_with_calc_period->events().size() == 5);
    REQUIRE(sched_with_calc_period->period().count() == 4);

    // TODO add more test cases
}

TEST_CASE("Schedule event sorting") {
    const std::filesystem::path testfiles[] = {"../tests/test_schedules/basic_test_schedule.json"};
    json schedule_description2 = json::parse(std::ifstream(testfiles[0]));
    schedule_description2["schedule"]["title"] = "aquarium#2";

    auto sched = schedule::deserialize(schedule_description2["schedule"]);

    REQUIRE(sched.has_value());

    auto events = sched->events();

    REQUIRE(events.size() == 4);
    REQUIRE(events[0].name() == "light_on");
    REQUIRE(events[1].name() == "light_off#3");
    REQUIRE(events[2].name() == "light_on#2");
    REQUIRE(events[3].name() == "light_off#2");
}
