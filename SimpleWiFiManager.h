/**************************************************************
   SimpleWiFiManager is a library for the ESP8266/Arduino platform
   (https://github.com/esp8266/Arduino) to enable easy
   configuration and reconfiguration of WiFi credentials using a Captive Portal
   based on the original WiFiManager https://github.com/tzapu/WiFiManager by AlexT https://github.com/tzapu
   Licensed under MIT license
 **************************************************************/

#ifndef SimpleWiFiManager_h
#define SimpleWiFiManager_h

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <memory>

extern "C" {
  #include "user_interface.h"
}

const char HTTP_HEAD[] PROGMEM            = "<!DOCTYPE html><html lang=\"en\"><head><meta charset=\"UTF-8\" name=\"viewport\" content=\"width=device-width, initial-scale=1, user-scalable=no\"/><title>{v}</title>";
const char HTTP_STYLE[] PROGMEM           = "<style>.c{text-align: center;} div,input{padding:5px;font-size:1em;} input{width:95%;} body{text-align: center;font-family:verdana;} button{border:0;border-radius:0.3rem;background-color:#1fa3ec;color:#fff;line-height:2.4rem;font-size:1.2rem;width:100%;} .q{float: right;width: 64px;text-align: right;} .l{background: url(\"data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAACAAAAAgCAMAAABEpIrGAAAALVBMVEX///8EBwfBwsLw8PAzNjaCg4NTVVUjJiZDRUUUFxdiZGSho6OSk5Pg4eFydHTCjaf3AAAAZElEQVQ4je2NSw7AIAhEBamKn97/uMXEGBvozkWb9C2Zx4xzWykBhFAeYp9gkLyZE0zIMno9n4g19hmdY39scwqVkOXaxph0ZCXQcqxSpgQpONa59wkRDOL93eAXvimwlbPbwwVAegLS1HGfZAAAAABJRU5ErkJggg==\") no-repeat left center;background-size: 1em;}</style>";
const char HTTP_SCRIPT[] PROGMEM          = "<script>function c(l){document.getElementById('s').value=l.innerText||l.textContent;document.getElementById('p').focus();}</script>";
const char HTTP_HEAD_END[] PROGMEM        = "</head><body><div style='text-align:left;display:inline-block;min-width:260px;'>";
const char HTTP_PORTAL_OPTIONS[] PROGMEM  = "<form action=\"/wifi\" method=\"get\"><button>Configure WiFi</button></form><br/><form action=\"/0wifi\" method=\"get\"><button>Configure WiFi (No Scan)</button></form><br/><form action=\"/i\" method=\"get\"><button>Info</button></form><br/><form action=\"/r\" method=\"post\"><button>Reset</button></form>";
const char HTTP_ITEM[] PROGMEM            = "<div><a href='#p' onclick='c(this)'>{v}</a>&nbsp;<span class='q {i}'>{r}%</span></div>";
const char HTTP_FORM_START[] PROGMEM      = "<form method='get' action='wifisave'><input id='s' name='s' length=32 placeholder='SSID'><br/><input id='p' name='p' length=64 type='password' placeholder='password'><br/>";
const char HTTP_FORM_PARAM[] PROGMEM      = "<br/><input id='{i}' name='{n}' maxlength={l} placeholder='{p}' value='{v}' {c}>";
const char HTTP_FORM_END[] PROGMEM        = "<br/><button type='submit'>save</button></form>";
const char HTTP_SCAN_LINK[] PROGMEM       = "<br/><div class=\"c\"><a href=\"/wifi\">Scan</a></div>";
const char HTTP_SAVED[] PROGMEM           = "<div>Credentials Saved<br />Trying to connect ESP to network.<br />If it fails reconnect to AP to try again</div>";
const char HTTP_END[] PROGMEM             = "</div></body></html>";

const char DEFAULT_APNAME[] PROGMEM       = "no-net";

class SimpleWiFiManager
{
  public:
	SimpleWiFiManager();
	~SimpleWiFiManager();

	//Trys to connect with known data or starts connect-portal.
	//Returns true, if the connection was successfull.
	//Returns false, if the connect-process is already running.
	//Returns false, if the connection was not successfull and the connect-portal was started.
	boolean			autoConnect();
	//Trys to connect with known data or starts connect-portal.
	//Returns true, if the connection was successfull.
	//Returns false, if the connect-process is already running.
	//Returns false, if the connection was not successfull and the connect-portal was started.
	boolean			autoConnect(char const *apName, char const *apPassword = NULL);

	//If you want to always start the config portal, without trying to connect first.
	//Returns true, is the connect-process was started.
	//Returns false, if the connect-process was already running.
	boolean			startConfigPortal();
	//If you want to always start the config portal, without trying to connect first.
	//Returns true, is the connect-process was started.
	//Returns false, if the connect-process was already running.
	boolean			startConfigPortal(char const *apName, char const *apPassword = NULL);

	// get the AP name of the config portal, so it can be used in the callback
	String			getConfigPortalSSID();

	void			resetSettings();

	//sets timeout for which to attempt connecting, useful if you get a lot of failed connects
	void			setConnectTimeout(unsigned long seconds);


	void			setDebugOutput(boolean debug);
	//defaults to not showing anything under 8% signal quality if called
	void			setMinimumSignalQuality(int quality = 8);
	//sets a custom ip /gateway /subnet configuration
	void			setAPStaticIPConfig(IPAddress ip, IPAddress gw, IPAddress sn);
	//sets config for a static IP
	void			setSTAStaticIPConfig(IPAddress ip, IPAddress gw, IPAddress sn);
	//called when AP mode and config portal is started
	inline void		setAPCallback( void (*func)(SimpleWiFiManager*) );
	//if this is set, try WPS setup when starting (this will delay config portal for up to 2 mins)
	//TODO
	//if this is set, customise style
	inline void		setCustomHeadElement(const char* element);
	//if this is true, remove duplicated Access Points - defaut true
	inline void		setRemoveDuplicateAPs(boolean removeDuplicates);

	//Check if there are clients connected to the AP
	inline bool		HasConnectedClients() {
		return wifi_softap_get_station_num() != 0;
	}

	//Call this function periodicaly
	boolean			HandleConnecting();
	//Use this to abort a running connect-process
	void			FinishConnecting();

	//Use this function to check, if the manager ic currently trying to connect
	boolean			IsConnecting();

	//This function gets the time since the last HTTP-handling was done in milliseconds.
	//Returns UINT32_MAX, if no HTTP-handling was done before.
	inline uint32_t MillisSinceLastPortalUsage();

  private:
	std::unique_ptr<DNSServer>        dnsServer;
	std::unique_ptr<ESP8266WebServer> server;

	//const int     WM_DONE                 = 0;
	//const int     WM_WAIT                 = 10;

	//const String  HTTP_HEAD = "<!DOCTYPE html><html lang=\"en\"><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"/><title>{v}</title>";

	void			setupConfigPortal();
	void			startWPS();
	void			cacheAP(const char* const Name, const char* const Password);

	const char*		_apName                 = NULL;
	const char*		_apPassword             = NULL;
	String			_ssid                   = "";
	String			_pass                   = "";
	unsigned long	_connectTimeout         = 0;

	uint32_t		_lastPortalHandle		= 0;

	IPAddress		_ap_static_ip;
	IPAddress		_ap_static_gw;
	IPAddress		_ap_static_sn;
	IPAddress		_sta_static_ip;
	IPAddress		_sta_static_gw;
	IPAddress		_sta_static_sn;

	int				_minimumQuality         = -1;
	boolean			_removeDuplicateAPs     = true;
	boolean			_tryWPS                 = false;

	const char*		_customHeadElement      = "";

	//String        getEEPROMString(int start, int len);
	//void          setEEPROMString(int start, int len, String string);

	int				status = WL_IDLE_STATUS;
	int				connectWifi(String ssid, String pass);
	uint8_t			waitForConnectResult();

	void			handleRoot();
	void			handleWifi(boolean scan);
	void			handleWifiSave();
	void			handleInfo();
	void			handleReset();
	void			handleNotFound();
	void			handle204();
	boolean			captivePortal();

	// DNS server
	const byte		DNS_PORT = 53;

	//helpers
	int				getRSSIasQuality(int RSSI);
	boolean			isIp(String str);
	String			toStringIp(IPAddress ip);

	boolean			connect;
	boolean			_debug = true;
	boolean			isConnecting = false;

	void (*_apcallback)(SimpleWiFiManager*) = NULL;
	void (*_savecallback)(void) = NULL;

	template <typename Generic>
	void			DEBUG_WM(Generic text);

	template <class T>
	auto optionalIPFromString(T *obj, const char *s) -> decltype(  obj->fromString(s)  ) {
	  return  obj->fromString(s);
	}
	auto optionalIPFromString(...) -> bool {
	  DEBUG_WM("NO fromString METHOD ON IPAddress, you need ESP8266 core 2.1.0 or newer for Custom IP configuration to work.");
	  return false;
	}
};

uint32_t SimpleWiFiManager::MillisSinceLastPortalUsage() {
	if (_lastPortalHandle == 0) { return UINT32_MAX; }
	return millis() - _lastPortalHandle;
}

//start up config portal callback
inline void SimpleWiFiManager::setAPCallback(void(*func)(SimpleWiFiManager* wiFiManager)) {
	_apcallback = func;
}

//sets a custom element to add to head, like a new style tag
inline void SimpleWiFiManager::setCustomHeadElement(const char* element) {
	_customHeadElement = element;
}

//if this is true, remove duplicated Access Points - defaut true
inline void SimpleWiFiManager::setRemoveDuplicateAPs(boolean removeDuplicates) {
	_removeDuplicateAPs = removeDuplicates;
}
#endif