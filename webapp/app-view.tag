<app-view>
    <home-view if={current_page == 0}></home-view>
    <schedule-view if={current_page == 1}></schedule-view>
    <manual-control-view if={current_page == 2} ></manual-control-view>

    <script>
        var self = this;
        self.current_page = 0;

        var r = route.create()
        r(show_home) 
        r('', show_home)
        r('show-home', show_home)
        r('show-schedules', show_schedules)
        r('show-manual-control', show_manual_control)
        r('show-stats', show_stats)
        r('show-logs', show_logs)

        function show_home() {
            self.update({current_page : 0});
        }

        function show_schedules() {
            self.update({current_page : 1});
        }

        function show_manual_control() {
            self.update({current_page : 2});
        }

        function show_stats() {
            self.update({current_page : 3});
        }

        function show_logs() {
            self.update({current_page : 4});
        }
    </script>
</app-view>
