#include "gui/web/main_container.h"

#include "Wt/WBootstrapTheme.h"
#include "Wt/WMenu.h"

main_container::main_container(const Wt::WEnvironment &env) : Wt::WApplication(env) {
    setTitle("QuariumController");

    m_navbar = root()->addWidget(std::make_unique<Wt::WNavigationBar>());
    m_button = root()->addWidget(std::make_unique<Wt::WPushButton>("TestButton"));
    m_text = root()->addWidget(std::make_unique<Wt::WText>("Standard text"));

    auto theme = std::make_shared<Wt::WBootstrapTheme>();
    theme->setVersion(Wt::BootstrapVersion::v3);
    setTheme(theme);

    m_navbar->setTitle("QuariumController");
    m_navbar->setResponsive(true);

    auto *menu = m_navbar->addMenu(std::make_unique<Wt::WMenu>());
    auto *first = menu->addItem("Hello World");
    auto *second = menu->addItem("Hello World 2");

    auto first_lambda = [this]() { m_text->setText("Hello World was pressed"); };
    auto second_lambda = [this]() { m_text->setText("Hello World2 was pressed"); };

    first->triggered().connect(first_lambda);
    second->triggered().connect(second_lambda);

    auto button_handler = [this]() { m_text->setText("Button was pressed !"); };

    m_button->clicked().connect(button_handler);
}
