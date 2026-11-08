/* Controlled unit fixtures, not an ordinary-input or rendered playthrough.
   Reuse the asserted equivalent-situation comparison without shipped hooks. */
#define main faction_orders_unit_main
#include "../../../../tests/test_faction_orders.c"
#undef main
int main(void) {
    static const unsigned governments[] = {MOLTING_COURT, ECHO_PARLIAMENT, MOLTING_COURT,
                                           DIVIDED_CROWN, DIVIDED_CROWN};
    for (unsigned i = 0; i < 5; i++) {
        unsigned serial = i == 4 ? 2 : 1;
        Cell selected = deployment(governments[i], i == 2, serial);
        printf("{\"government\":%u,\"power_shortage\":%s,\"deployment\":%u,"
               "\"selected_goal\":[%d,%d,%d],\"assertions_passed\":true}\n",
               governments[i], i == 2 ? "true" : "false", serial, selected.x, selected.y,
               selected.z);
    }
    return 0;
}
