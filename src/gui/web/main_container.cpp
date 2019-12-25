#include "gui/web/main_container.h"

main_container::main_container(const Wt::WEnvironment &env) : Wt::WApplication(env) {
    setTitle("QuariumController");

    m_button = root()->addWidget(std::make_unique<Wt::WPushButton>("TestButton"));
    m_text = root()->addWidget(std::make_unique<Wt::WText>("Standard text"));

    auto button_handler = [this]() { m_text->setText("Button was pressed !"); };

    m_button->clicked().connect(button_handler);
}
