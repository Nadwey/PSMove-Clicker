#include <xtd/xtd>
#include <string>
#include "PSMC_ControllerManager.h"

using namespace xtd;
using namespace xtd::forms;

PSMC::ControllerManager* controllerManager;

class mainWindow : public form {
public:
    mainWindow() {
        xtd::drawing::font titleFont("Segoe UI Light", 23);
        xtd::drawing::font defaultFont("Segoe UI", 12);
        xtd::drawing::font smallFont("Segoe UI", 9);
        text("PSMove Clicker");
        size({ 435, 500 });

        mainTimer.interval_milliseconds(1000 / 100);
        mainTimer.enabled(true);
        mainTimer.tick += [&](object& sender, const event_args& e) {
            controllerManager->Process();
        };

        infoTimer.interval_milliseconds(1000 / 5);
        infoTimer.enabled(true);
        infoTimer.tick += [&](object& sender, const event_args& e) {
            status.text(controllerManager->GetControllerInfoString());
        };

        title.location({ 5, 5 });
        title.size({ 250, 50 });
        title.parent(*this);
        title.text("PSMove Clicker");
        title.font(titleFont);

        forceTextbox.location({ 5, 60 });
        forceTextbox.size({ 200, 30 });
        forceTextbox.font(defaultFont);
        forceTextbox.parent(*this);
        forceTextbox.text(std::to_string(controllerManager->force));
        forceTextbox.text_changed += [&] {
            try {
                controllerManager->force = std::stof(forceTextbox.text());
                forceTextbox.fore_color(xtd::drawing::color::white);
            }
            catch (...) {
                forceTextbox.fore_color(xtd::drawing::color::red);
            }
        };

        adofaiModeCheckbox.location({ 210, 60 });
        adofaiModeCheckbox.size({ 200, 30 });
        adofaiModeCheckbox.font(defaultFont);
        adofaiModeCheckbox.text("ADOFAI mode");
        adofaiModeCheckbox.parent(*this);
        adofaiModeCheckbox.checked(controllerManager->ADOFAI_Mode);
        adofaiModeCheckbox.check_state_changed += [&] {
            controllerManager->ADOFAI_Mode = adofaiModeCheckbox.checked();
        };

        status.location({ 5, 100 });
        status.size({ 410, 350 });
        status.multiline(true);
        status.read_only(true);
        status.parent(*this);
        status.font(smallFont);
    }

private:
    label title;
    timer mainTimer;
    timer infoTimer;
    text_box forceTextbox;
    check_box adofaiModeCheckbox;
    text_box status;
};

int main() {
    bool success = false;
    while (!success) {
        try {
            controllerManager = new PSMC::ControllerManager();
            success = true;
        }
        catch (...) {
            dialog_result msgBoxResult = message_box::show("Failed to connect with PSMoveService", "Error", message_box_buttons::retry_cancel, message_box_icon::error);
            if (msgBoxResult == dialog_result::cancel) return 1;
        }
    }
    application::run(mainWindow());
    delete controllerManager;
}