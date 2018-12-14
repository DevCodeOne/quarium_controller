#pragma once

enum struct page_event { ok = 0, error = -1 };

class page_interface {
   public:
    page_interface() = default;
    virtual ~page_interface() = default;

    virtual page_event enter_page() = 0;
    virtual page_event leave_page() = 0;
    virtual page_event update_contents() = 0;

   private:
};
