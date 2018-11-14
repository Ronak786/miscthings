import pywinauto
from pywinauto import Application

from modules.base.xobject import XObject


class FimeIntegriHelper(XObject):
    def __init__(self):
        super(FimeIntegriHelper, self).__init__()
        self._fime_app = None
        self._fime_app_mainwin = None

    def sync(self):
        try:
            if self._fime_app is None:
                self._fime_app = Application(backend='uia').connect(title_re='EVA*')
                self._fime_app_mainwin = self._fime_app.window(title_re='EVAL*')
                self._fime_app_mainwin.print_control_identifiers(depth=3, filename='_fime_app_mainwin.txt')
        except Exception as e:
            self.exception(e)
            return False
        return True

    def _click(self, ui_object):
        local = ui_object.element_info.rectangle
        ui_object.set_focus()
        pywinauto.mouse.click(coords=(local.left + 5, local.top + 5))

    def handle_pop_message(self):
        try:

            dlg_pop_message = self._fime_app_mainwin.window(title='POP Message', class_name='ThunderRT6UserControlDC')
            dlg_pop_message.wait('ready', timeout=20)
            self.info('dlg_pop_message 1')
            PassButton = self._fime_app_mainwin.window(title='Pass', control_type="Button", class_name='ThunderRT6CommandButton')
            self._click(PassButton.wrapper_object())
        except Exception as e:
            self.exception(e)
            self.info('Can not find window(POP Message)')
        else:
            try:
                PassButton = dlg_pop_message.window(title='Pass', control_type="Button")
                self._click(PassButton.wrapper_object())
            except Exception as e:
                self.exception(e)
                self.info('Can not click Pass Button')
