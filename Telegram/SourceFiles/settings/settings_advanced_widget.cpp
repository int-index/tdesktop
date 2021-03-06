/*
This file is part of Telegram Desktop,
the official desktop version of Telegram messaging app, see https://telegram.org

Telegram Desktop is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

It is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

In addition, as a special exception, the copyright holders give permission
to link the code of portions of this program with the OpenSSL library.

Full license: https://github.com/telegramdesktop/tdesktop/blob/master/LICENSE
Copyright (c) 2014-2017 John Preston, https://desktop.telegram.org
*/
#include "settings/settings_advanced_widget.h"

#include "styles/style_settings.h"
#include "lang.h"
#include "boxes/connectionbox.h"
#include "boxes/confirmbox.h"
#include "boxes/aboutbox.h"
#include "boxes/localstoragebox.h"
#include "mainwindow.h"
#include "ui/widgets/buttons.h"
#include "ui/effects/widget_slide_wrap.h"
#include "storage/localstorage.h"
#include "window/themes/window_theme.h"

namespace Settings {

AdvancedWidget::AdvancedWidget(QWidget *parent, UserData *self) : BlockWidget(parent, self, lang(lng_settings_section_advanced_settings)) {
	createControls();
#ifndef TDESKTOP_DISABLE_NETWORK_PROXY
	subscribe(Global::RefConnectionTypeChanged(), [this]() {
		connectionTypeUpdated();
	});
#endif // !TDESKTOP_DISABLE_NETWORK_PROXY
	if (!self) {
		subscribe(Window::Theme::Background(), [this](const Window::Theme::BackgroundUpdate &update) {
			if (update.type == Window::Theme::BackgroundUpdate::Type::ApplyingTheme) {
				checkNonDefaultTheme();
			}
		});
	}
}

void AdvancedWidget::createControls() {
	style::margins marginSmall(0, 0, 0, st::settingsSmallSkip);
	style::margins marginLarge(0, 0, 0, st::settingsLargeSkip);

	style::margins marginLocalStorage = ([&marginSmall, &marginLarge]() {
#ifndef TDESKTOP_DISABLE_NETWORK_PROXY
		return marginSmall;
#else // !TDESKTOP_DISABLE_NETWORK_PROXY
		return marginLarge;
#endif // TDESKTOP_DISABLE_NETWORK_PROXY
	})();
	if (self()) {
		addChildRow(_manageLocalStorage, marginLocalStorage, lang(lng_settings_manage_local_storage), SLOT(onManageLocalStorage()));
	}

#ifndef TDESKTOP_DISABLE_NETWORK_PROXY
	addChildRow(_connectionType, marginLarge, lang(lng_connection_type), lang(lng_connection_auto_connecting), LabeledLink::Type::Primary, SLOT(onConnectionType()));
	connectionTypeUpdated();
#endif // !TDESKTOP_DISABLE_NETWORK_PROXY

	if (self()) {
		addChildRow(_askQuestion, marginSmall, lang(lng_settings_ask_question), SLOT(onAskQuestion()));
	} else {
		style::margins slidedPadding(0, marginLarge.bottom() / 2, 0, marginLarge.bottom() - (marginLarge.bottom() / 2));
		addChildRow(_useDefaultTheme, marginLarge, slidedPadding, lang(lng_settings_bg_use_default), SLOT(onUseDefaultTheme()));
		if (!Local::hasTheme()) {
			_useDefaultTheme->hideFast();
		}
	}
	addChildRow(_telegramFAQ, marginLarge, lang(lng_settings_faq), SLOT(onTelegramFAQ()));
	if (self()) {
		style::margins marginLogout(0, 0, 0, 2 * st::settingsLargeSkip);
		addChildRow(_logOut, marginLogout, lang(lng_settings_logout), SLOT(onLogOut()));
	}
}

void AdvancedWidget::checkNonDefaultTheme() {
	if (self()) return;
	_useDefaultTheme->toggleAnimated(Local::hasTheme());
}

void AdvancedWidget::onManageLocalStorage() {
	Ui::show(Box<LocalStorageBox>());
}

#ifndef TDESKTOP_DISABLE_NETWORK_PROXY
void AdvancedWidget::connectionTypeUpdated() {
	auto connection = [] {
		switch (Global::ConnectionType()) {
		case dbictHttpProxy:
		case dbictTcpProxy: {
			auto transport = MTP::dctransport();
			return transport.isEmpty() ? lang(lng_connection_proxy_connecting) : lng_connection_proxy(lt_transport, transport);
		} break;
		case dbictAuto:
		default: {
			auto transport = MTP::dctransport();
			return transport.isEmpty() ? lang(lng_connection_auto_connecting) : lng_connection_auto(lt_transport, transport);
		} break;
		}
	};
	_connectionType->link()->setText(connection());
	resizeToWidth(width());
}

void AdvancedWidget::onConnectionType() {
	Ui::show(Box<ConnectionBox>());
}
#endif // !TDESKTOP_DISABLE_NETWORK_PROXY

void AdvancedWidget::onUseDefaultTheme() {
	Window::Theme::ApplyDefault();
}

void AdvancedWidget::onAskQuestion() {
	auto box = Box<ConfirmBox>(lang(lng_settings_ask_sure), lang(lng_settings_ask_ok), lang(lng_settings_faq_button), base::lambda_guarded(this, [this] {
		onAskQuestionSure();
	}), base::lambda_guarded(this, [this] {
		onTelegramFAQ();
	}));
	box->setStrictCancel(true);
	Ui::show(std::move(box));
}

void AdvancedWidget::onAskQuestionSure() {
	if (_supportGetRequest) return;
	_supportGetRequest = MTP::send(MTPhelp_GetSupport(), rpcDone(&AdvancedWidget::supportGot));
}

void AdvancedWidget::supportGot(const MTPhelp_Support &support) {
	if (!App::main()) return;

	if (support.type() == mtpc_help_support) {
		if (auto user = App::feedUsers(MTP_vector<MTPUser>(1, support.c_help_support().vuser))) {
			Ui::showPeerHistory(user, ShowAtUnreadMsgId);
		}
	}
}

void AdvancedWidget::onTelegramFAQ() {
	QDesktopServices::openUrl(telegramFaqLink());
}

void AdvancedWidget::onLogOut() {
	App::wnd()->onLogout();
}

} // namespace Settings
