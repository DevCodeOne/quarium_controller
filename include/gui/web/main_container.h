#pragma once

#include "Wt/WApplication.h"
#include "Wt/WPushButton.h"
#include "Wt/WText.h"

class main_container : public Wt::WApplication {
   public:
    main_container(const Wt::WEnvironment &env);
    virtual ~main_container() = default;

   private:
    Wt::WPushButton *m_button;
    Wt::WText *m_text;
};
