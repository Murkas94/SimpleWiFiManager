/**************************************************************
   SimpleWiFiManager is a library for the ESP8266/Arduino platform
   (https://github.com/esp8266/Arduino) to enable easy
   configuration and reconfiguration of WiFi credentials using a Captive Portal
   based on the original WiFiManager https://github.com/tzapu/WiFiManager by AlexT https://github.com/tzapu
   Licensed under MIT license
 **************************************************************/

#include "SimpleWiFiManager.h"

SimpleWiFiManager::SimpleWiFiManager() {
	int len = strlen_P(DEFAULT_APNAME) + 1;
	char* apName = new char[len];
	memcpy_P(&apName[0], DEFAULT_APNAME, len);
	_apName = apName;
}

SimpleWiFiManager::~SimpleWiFiManager()
{
	delete[] _apName;
	if (_apPassword != nullptr) {
		delete[] _apPassword;
	}
}

void SimpleWiFiManager::cacheAP(const char* const Name, const char* const Password) {
	delete[] _apName;
	int len = strlen(Name) + 1;
	char* buf = new char[len];
	memcpy(&buf[0], Name, len);
	_apName = buf;

	if (_apPassword != nullptr) {
		delete[] _apPassword;
	}
	if (Password != nullptr) {
		len = strlen(Password) + 1;
		buf = new char[len];
		memcpy(&buf[0], Password, len);
		_apPassword = buf;
	}
	else {
		_apPassword = nullptr;
	}
}

void SimpleWiFiManager::setupConfigPortal() {
	dnsServer.reset(new DNSServer());
	server.reset(new ESP8266WebServer(80));

	DEBUG_WM(F(""));

	DEBUG_WM(F("Configuring access point... "));
	DEBUG_WM(_apName);
	if (_apPassword != NULL) {
		if (strlen(_apPassword) < 8 || strlen(_apPassword) > 63) {
			// fail passphrase to short or long!
			DEBUG_WM(F("Invalid AccessPoint password. Ignoring"));
			_apPassword = NULL;
		}
		DEBUG_WM(_apPassword);
	}

	//optional soft ip config
	if (_ap_static_ip) {
		DEBUG_WM(F("Custom AP IP/GW/Subnet"));
		WiFi.softAPConfig(_ap_static_ip, _ap_static_gw, _ap_static_sn);
	}

	if (_apPassword != NULL) {
		WiFi.softAP(_apName, _apPassword);//password option
	}
	else {
		WiFi.softAP(_apName);
	}

	delay(500); // Without delay I've seen the IP address blank
	DEBUG_WM(F("AP IP address: "));
	DEBUG_WM(WiFi.softAPIP());

	/* Setup the DNS server redirecting all the domains to the apIP */
	dnsServer->setErrorReplyCode(DNSReplyCode::NoError);
	dnsServer->start(DNS_PORT, "*", WiFi.softAPIP());

	/* Setup web pages: root, wifi config pages, SO captive portal detectors and not found. */
	server->on(String(F("/")), std::bind(&SimpleWiFiManager::handleRoot, this));
	server->on(String(F("/wifi")), std::bind(&SimpleWiFiManager::handleWifi, this, true));
	server->on(String(F("/0wifi")), std::bind(&SimpleWiFiManager::handleWifi, this, false));
	server->on(String(F("/wifisave")), std::bind(&SimpleWiFiManager::handleWifiSave, this));
	server->on(String(F("/i")), std::bind(&SimpleWiFiManager::handleInfo, this));
	server->on(String(F("/r")), std::bind(&SimpleWiFiManager::handleReset, this));
	//server->on("/generate_204", std::bind(&SimpleWiFiManager::handle204, this));  //Android/Chrome OS captive portal check.
	server->on(String(F("/fwlink")), std::bind(&SimpleWiFiManager::handleRoot, this));  //Microsoft captive portal. Maybe not needed. Might be handled by notFound handler.
	server->onNotFound(std::bind(&SimpleWiFiManager::handleNotFound, this));
	server->begin(); // Web server start
	DEBUG_WM(F("HTTP server started"));

}

boolean SimpleWiFiManager::autoConnect() {
	String ssid = "ESP" + String(ESP.getChipId());
	return autoConnect(ssid.c_str(), NULL);
}

boolean SimpleWiFiManager::autoConnect(char const *apName, char const *apPassword) {
	if (isConnecting) { return false; }
	DEBUG_WM(F(""));
	DEBUG_WM(F("AutoConnect"));

	// read eeprom for ssid and pass
	//String ssid = getSSID();
	//String pass = getPassword();

	// attempt to connect; should it fail, fall back to AP
	WiFi.mode(WIFI_STA);

	if (connectWifi("", "") == WL_CONNECTED) {
		DEBUG_WM(F("IP Address:"));
		DEBUG_WM(WiFi.localIP());
		//connected
		return true;
	}

	startConfigPortal(apName, apPassword);
	return false;
}

boolean SimpleWiFiManager::startConfigPortal() {
	String ssid = "ESP" + String(ESP.getChipId());
	return startConfigPortal(ssid.c_str(), NULL);
}

boolean  SimpleWiFiManager::startConfigPortal(char const *apName, char const *apPassword) {
	if (isConnecting) { return false; }

	if (!WiFi.isConnected()) {
		WiFi.persistent(false);
		// disconnect sta, start ap
		WiFi.disconnect(); //  this alone is not enough to stop the autoconnecter
		WiFi.mode(WIFI_AP);
		WiFi.persistent(true);
	}
	else {
		//setup AP
		WiFi.mode(WIFI_AP_STA);
		DEBUG_WM(F("SET AP STA"));
	}

	//Save the given apName and apPassword for later usage
	cacheAP(apName, apPassword);

	//notify we entered AP mode
	if (_apcallback != NULL) {
		_apcallback(this);
	}

	connect = false;
	isConnecting = true;
	setupConfigPortal();

	return true;
}

boolean SimpleWiFiManager::HandleConnecting() {
	if (isConnecting == false) { return false; }

	//DNS
	dnsServer->processNextRequest();
	//HTTP
	server->handleClient();

	//connect gets set by the http-server
	if (connect) {
		connect = false;
		delay(2000);
		DEBUG_WM(F("Connecting to new AP"));

		// using user-provided  _ssid, _pass in place of system-stored ssid and pass
		if (connectWifi(_ssid, _pass) != WL_CONNECTED) {
			DEBUG_WM(F("Failed to connect."));
			return false;
		}
		else {
			//connected
			WiFi.mode(WIFI_STA);
			FinishConnecting();
			return true;
		}
	}

	return false;
}

void SimpleWiFiManager::FinishConnecting() {
	if (isConnecting) {
		server.reset();
		dnsServer.reset();
	}
	WiFi.softAPdisconnect(true);
	isConnecting = false;
}

boolean SimpleWiFiManager::IsConnecting() {
	return isConnecting;
}

int SimpleWiFiManager::connectWifi(String ssid, String pass) {
	DEBUG_WM(F("Connecting as wifi client..."));

	// check if we've got static_ip settings, if we do, use those.
	if (_sta_static_ip) {
		DEBUG_WM(F("Custom STA IP/GW/Subnet"));
		WiFi.config(_sta_static_ip, _sta_static_gw, _sta_static_sn);
		DEBUG_WM(WiFi.localIP());
	}
	//fix for auto connect racing issue
	if (WiFi.status() == WL_CONNECTED) {
		DEBUG_WM(F("Already connected. Bailing out."));
		return WL_CONNECTED;
	}
	//check if we have ssid and pass and force those, if not, try with last saved values
	if (ssid != "") {
		WiFi.begin(ssid.c_str(), pass.c_str());
	}
	else {
		if (WiFi.SSID()) {
			DEBUG_WM(F("Using last saved values, should be faster"));
			//trying to fix connection in progress hanging
			ETS_UART_INTR_DISABLE();
			wifi_station_disconnect();
			ETS_UART_INTR_ENABLE();

			WiFi.begin();
		}
		else {
			DEBUG_WM(F("No saved credentials"));
		}
	}

	int connRes = waitForConnectResult();
	DEBUG_WM("Connection result: ");
	DEBUG_WM(connRes);
	//not connected, WPS enabled, no pass - first attempt
#ifdef NO_EXTRA_4K_HEAP
	if (_tryWPS && connRes != WL_CONNECTED && pass == "") {
		startWPS();
		//should be connected at the end of WPS
		connRes = waitForConnectResult();
	}
#endif
	return connRes;
}

uint8_t SimpleWiFiManager::waitForConnectResult() {
	if (_connectTimeout == 0) {
		return WiFi.waitForConnectResult();
	}
	else {
		DEBUG_WM(F("Waiting for connection result with time out"));
		unsigned long start = millis();
		boolean keepConnecting = true;
		uint8_t status;
		while (keepConnecting) {
			status = WiFi.status();
			if (millis() > start + _connectTimeout) {
				keepConnecting = false;
				DEBUG_WM(F("Connection timed out"));
			}
			if (status == WL_CONNECTED || status == WL_CONNECT_FAILED) {
				keepConnecting = false;
			}
			delay(100);
		}
		return status;
	}
}

void SimpleWiFiManager::startWPS() {
	DEBUG_WM(F("START WPS"));
	WiFi.beginWPSConfig();
	DEBUG_WM(F("END WPS"));
}
/*
  String SimpleWiFiManager::getSSID() {
  if (_ssid == "") {
	DEBUG_WM(F("Reading SSID"));
	_ssid = WiFi.SSID();
	DEBUG_WM(F("SSID: "));
	DEBUG_WM(_ssid);
  }
  return _ssid;
  }

  String SimpleWiFiManager::getPassword() {
  if (_pass == "") {
	DEBUG_WM(F("Reading Password"));
	_pass = WiFi.psk();
	DEBUG_WM("Password: " + _pass);
	//DEBUG_WM(_pass);
  }
  return _pass;
  }
*/
String SimpleWiFiManager::getConfigPortalSSID() {

}

void SimpleWiFiManager::resetSettings() {
	DEBUG_WM(F("settings invalidated"));
	DEBUG_WM(F("THIS MAY CAUSE AP NOT TO START UP PROPERLY. YOU NEED TO COMMENT IT OUT AFTER ERASING THE DATA."));
	WiFi.disconnect(true);
	//delay(200);
}

void SimpleWiFiManager::setConnectTimeout(unsigned long seconds) {
	_connectTimeout = seconds * 1000;
}

void SimpleWiFiManager::setDebugOutput(boolean debug) {
	_debug = debug;
}

void SimpleWiFiManager::setAPStaticIPConfig(IPAddress ip, IPAddress gw, IPAddress sn) {
	_ap_static_ip = ip;
	_ap_static_gw = gw;
	_ap_static_sn = sn;
}

void SimpleWiFiManager::setSTAStaticIPConfig(IPAddress ip, IPAddress gw, IPAddress sn) {
	_sta_static_ip = ip;
	_sta_static_gw = gw;
	_sta_static_sn = sn;
}

void SimpleWiFiManager::setMinimumSignalQuality(int quality) {
	_minimumQuality = quality;
}

/** Handle root or redirect to captive portal */
void SimpleWiFiManager::handleRoot() {
	_lastPortalHandle = millis();
	DEBUG_WM(F("Handle root"));
	if (captivePortal()) { // If caprive portal redirect instead of displaying the page.
		return;
	}

	String page = FPSTR(HTTP_HEAD);
	page.replace("{v}", "Options");
	page += FPSTR(HTTP_SCRIPT);
	page += FPSTR(HTTP_STYLE);
	page += _customHeadElement;
	page += FPSTR(HTTP_HEAD_END);
	page += String(F("<h1>"));
	page += _apName;
	page += String(F("</h1>"));
	page += String(F("<h3>WiFiManager</h3>"));
	page += FPSTR(HTTP_PORTAL_OPTIONS);
	page += FPSTR(HTTP_END);

	server->sendHeader("Content-Length", String(page.length()));
	server->send(200, "text/html", page);

}

/** Wifi config page handler */
void SimpleWiFiManager::handleWifi(boolean scan) {
	_lastPortalHandle = millis();
	String page = FPSTR(HTTP_HEAD);
	page.replace("{v}", "Config ESP");
	page += FPSTR(HTTP_SCRIPT);
	page += FPSTR(HTTP_STYLE);
	page += _customHeadElement;
	page += FPSTR(HTTP_HEAD_END);

	if (scan) {
		int n = WiFi.scanNetworks();
		DEBUG_WM(F("Scan done"));
		if (n == 0) {
			DEBUG_WM(F("No networks found"));
			page += F("No networks found. Refresh to scan again.");
		}
		else {
			//sort networks
			int* indices = new int[n];
			for (int i = 0; i < n; i++) {
				indices[i] = i;
			}

			// RSSI SORT

			// old sort
			for (int i = 0; i < n; i++) {
				for (int j = i + 1; j < n; j++) {
					if (WiFi.RSSI(indices[j]) > WiFi.RSSI(indices[i])) {
						std::swap(indices[i], indices[j]);
					}
				}
			}

			/*std::sort(indices, indices + n, [](const int & a, const int & b) -> bool
				{
				return WiFi.RSSI(a) > WiFi.RSSI(b);
				});*/

				// remove duplicates ( must be RSSI sorted )
			if (_removeDuplicateAPs) {
				String cssid;
				for (int i = 0; i < n; i++) {
					if (indices[i] == -1) continue;
					cssid = WiFi.SSID(indices[i]);
					for (int j = i + 1; j < n; j++) {
						if (cssid == WiFi.SSID(indices[j])) {
							DEBUG_WM("DUP AP: " + WiFi.SSID(indices[j]));
							indices[j] = -1; // set dup aps to index -1
						}
					}
				}
			}

			//display networks in page
			for (int i = 0; i < n; i++) {
				if (indices[i] == -1) continue; // skip dups
				DEBUG_WM(WiFi.SSID(indices[i]));
				DEBUG_WM(WiFi.RSSI(indices[i]));
				int quality = getRSSIasQuality(WiFi.RSSI(indices[i]));

				if (_minimumQuality == -1 || _minimumQuality < quality) {
					String item = FPSTR(HTTP_ITEM);
					String rssiQ;
					rssiQ += quality;
					item.replace("{v}", WiFi.SSID(indices[i]));
					item.replace("{r}", rssiQ);
					if (WiFi.encryptionType(indices[i]) != ENC_TYPE_NONE) {
						item.replace("{i}", "l");
					}
					else {
						item.replace("{i}", "");
					}
					//DEBUG_WM(item);
					page += item;
					delay(0);
				}
				else {
					DEBUG_WM(F("Skipping due to quality"));
				}
			}
			page += "<br/>";
			
			delete[] indices;
		}
	}

	page += FPSTR(HTTP_FORM_START);

	if (_sta_static_ip) {

		String item = FPSTR(HTTP_FORM_PARAM);
		item.replace("{i}", "ip");
		item.replace("{n}", "ip");
		item.replace("{p}", "Static IP");
		item.replace("{l}", "15");
		item.replace("{v}", _sta_static_ip.toString());

		page += item;

		item = FPSTR(HTTP_FORM_PARAM);
		item.replace("{i}", "gw");
		item.replace("{n}", "gw");
		item.replace("{p}", "Static Gateway");
		item.replace("{l}", "15");
		item.replace("{v}", _sta_static_gw.toString());

		page += item;

		item = FPSTR(HTTP_FORM_PARAM);
		item.replace("{i}", "sn");
		item.replace("{n}", "sn");
		item.replace("{p}", "Subnet");
		item.replace("{l}", "15");
		item.replace("{v}", _sta_static_sn.toString());

		page += item;

		page += "<br/>";
	}

	page += FPSTR(HTTP_FORM_END);
	page += FPSTR(HTTP_SCAN_LINK);

	page += FPSTR(HTTP_END);

	server->sendHeader("Content-Length", String(page.length()));
	server->send(200, "text/html", page);


	DEBUG_WM(F("Sent config page"));
}

/** Handle the WLAN save form and redirect to WLAN config page again */
void SimpleWiFiManager::handleWifiSave() {
	_lastPortalHandle = millis();
	DEBUG_WM(F("WiFi save"));

	//SAVE/connect here
	_ssid = server->arg("s").c_str();
	_pass = server->arg("p").c_str();

	if (server->arg("ip") != "") {
		DEBUG_WM(F("static ip"));
		DEBUG_WM(server->arg("ip"));
		//_sta_static_ip.fromString(server->arg("ip"));
		String ip = server->arg("ip");
		optionalIPFromString(&_sta_static_ip, ip.c_str());
	}
	if (server->arg("gw") != "") {
		DEBUG_WM(F("static gateway"));
		DEBUG_WM(server->arg("gw"));
		String gw = server->arg("gw");
		optionalIPFromString(&_sta_static_gw, gw.c_str());
	}
	if (server->arg("sn") != "") {
		DEBUG_WM(F("static netmask"));
		DEBUG_WM(server->arg("sn"));
		String sn = server->arg("sn");
		optionalIPFromString(&_sta_static_sn, sn.c_str());
	}

	String page = FPSTR(HTTP_HEAD);
	page.replace("{v}", "Credentials Saved");
	page += FPSTR(HTTP_SCRIPT);
	page += FPSTR(HTTP_STYLE);
	page += _customHeadElement;
	page += FPSTR(HTTP_HEAD_END);
	page += FPSTR(HTTP_SAVED);
	page += FPSTR(HTTP_END);

	server->sendHeader("Content-Length", String(page.length()));
	server->send(200, "text/html", page);

	DEBUG_WM(F("Sent wifi save page"));

	connect = true; //signal ready to connect/reset
}

/** Handle the info page */
void SimpleWiFiManager::handleInfo() {
	_lastPortalHandle = millis();
	DEBUG_WM(F("Info"));

	String page = FPSTR(HTTP_HEAD);
	page.replace("{v}", "Info");
	page += FPSTR(HTTP_SCRIPT);
	page += FPSTR(HTTP_STYLE);
	page += _customHeadElement;
	page += FPSTR(HTTP_HEAD_END);
	page += F("<dl>");
	page += F("<dt>Chip ID</dt><dd>");
	page += ESP.getChipId();
	page += F("</dd>");
	page += F("<dt>Flash Chip ID</dt><dd>");
	page += ESP.getFlashChipId();
	page += F("</dd>");
	page += F("<dt>IDE Flash Size</dt><dd>");
	page += ESP.getFlashChipSize();
	page += F(" bytes</dd>");
	page += F("<dt>Real Flash Size</dt><dd>");
	page += ESP.getFlashChipRealSize();
	page += F(" bytes</dd>");
	page += F("<dt>Soft AP IP</dt><dd>");
	page += WiFi.softAPIP().toString();
	page += F("</dd>");
	page += F("<dt>Soft AP MAC</dt><dd>");
	page += WiFi.softAPmacAddress();
	page += F("</dd>");
	page += F("<dt>Station MAC</dt><dd>");
	page += WiFi.macAddress();
	page += F("</dd>");
	page += F("</dl>");
	page += FPSTR(HTTP_END);

	server->sendHeader("Content-Length", String(page.length()));
	server->send(200, "text/html", page);

	DEBUG_WM(F("Sent info page"));
}

/** Handle the reset page */
void SimpleWiFiManager::handleReset() {
	_lastPortalHandle = millis();
	DEBUG_WM(F("Reset"));

	String page = FPSTR(HTTP_HEAD);
	page.replace("{v}", "Info");
	page += FPSTR(HTTP_SCRIPT);
	page += FPSTR(HTTP_STYLE);
	page += _customHeadElement;
	page += FPSTR(HTTP_HEAD_END);
	page += F("Module will reset in a few seconds.");
	page += FPSTR(HTTP_END);

	server->sendHeader("Content-Length", String(page.length()));
	server->send(200, "text/html", page);

	DEBUG_WM(F("Sent reset page"));
	delay(5000);
	ESP.reset();
	delay(2000);
}

void SimpleWiFiManager::handleNotFound() {
	_lastPortalHandle = millis();
	if (captivePortal()) { // If captive portal redirect instead of displaying the error page.
		return;
	}
	String message = "File Not Found\n\n";
	message += "URI: ";
	message += server->uri();
	message += "\nMethod: ";
	message += (server->method() == HTTP_GET) ? "GET" : "POST";
	message += "\nArguments: ";
	message += server->args();
	message += "\n";

	for (uint8_t i = 0; i < server->args(); i++) {
		message += " " + server->argName(i) + ": " + server->arg(i) + "\n";
	}
	server->sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
	server->sendHeader("Pragma", "no-cache");
	server->sendHeader("Expires", "-1");
	server->sendHeader("Content-Length", String(message.length()));
	server->send(404, "text/plain", message);
}


/** Redirect to captive portal if we got a request for another domain. Return true in that case so the page handler do not try to handle the request again. */
boolean SimpleWiFiManager::captivePortal() {
	_lastPortalHandle = millis();
	if (!isIp(server->hostHeader())) {
		DEBUG_WM(F("Request redirected to captive portal"));
		server->sendHeader("Location", String("http://") + toStringIp(server->client().localIP()), true);
		server->send(302, "text/plain", ""); // Empty content inhibits Content-length header so we have to close the socket ourselves.
		server->client().stop(); // Stop is needed because we sent no content length
		return true;
	}
	return false;
}

template <typename Generic>
void SimpleWiFiManager::DEBUG_WM(Generic text) {
	if (_debug) {
		Serial.print("*WM: ");
		Serial.println(text);
	}
}

int SimpleWiFiManager::getRSSIasQuality(int RSSI) {
	int quality = 0;

	if (RSSI <= -100) {
		quality = 0;
	}
	else if (RSSI >= -50) {
		quality = 100;
	}
	else {
		quality = 2 * (RSSI + 100);
	}
	return quality;
}

/** Is this an IP? */
boolean SimpleWiFiManager::isIp(String str) {
	for (size_t i = 0; i < str.length(); i++) {
		int c = str.charAt(i);
		if (c != '.' && (c < '0' || c > '9')) {
			return false;
		}
	}
	return true;
}

/** IP to String? */
String SimpleWiFiManager::toStringIp(IPAddress ip) {
	String res = "";
	for (int i = 0; i < 3; i++) {
		res += String((ip >> (8 * i)) & 0xFF) + ".";
	}
	res += String(((ip >> 8 * 3)) & 0xFF);
	return res;
}
