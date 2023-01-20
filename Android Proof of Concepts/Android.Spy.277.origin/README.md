# Spy277
![Android](https://img.shields.io/badge/Android-3DDC84?style=for-the-badge&logo=android&logoColor=white)

SHA1: 89dee7086968dda688703304fd4b5163476b7f44

A Trojan for Android that steals confidential information and delivers advertisements. It is distributed via bogus versions of popular Android applications on the Google Play store.

Once launched, it sends the following information to the command and control server:

    “email”—email address connected to a Google user account. It is obtained from AccountManager; at that, only getAccountsByType("com.google”) is used;
    “app_id”—meta-data from the manifest.xml file of the infected application;
    “gcm_id”—GCM identifier (Google Cloud Messaging id);
    “sender_id”—the application identifier specified by the server;
    “app_version_code”—android:versionCode from the manifest.xml file;
    “package_name”—name of the package containing the Trojan;
    “sdk_version_name”—SDK version;
    “sdk_version_code”—the Trojan’s SDK build number;
    “android_id”—unique identifier;
    “imei”—IMEI identifier;
    “android_version”—operating system version;
    “model”—mobile device model;
    “screen”—screen resolution;
    “phone”—cell phone number;
    “api_version”—SDK version of the application;
    “country”—the user’s geolocation;
    “cpu”—CPU type;
    “mac”—MAC address of the power adapter;
    “user_agent”—generates User Agent line for a certain device, considering such parameters as Build.VERSION.RELEASE, Build.VERSION.CODENAME, and Build.MODEL, Build.ID;
    “carrier”—mobile network operator;
    “is_plugin”—informs the server whether the Trojan is launched from the original aaplication (the false value) or from its plug-in located in its software resources (the false value);
    “time_from_begin”—specifies a time period after which the Trojan starts transmitting data to the server;
    “install_referrer”—by default, sends the "Others” value;
    “connection_type”—network connection type;
    “connection_sub_type”—network subtype;
    “is_rooted”—availability of root access (the true or false value);
    “active_device_admin”—indicates whether an infected application has administrator privileges (the true or false value);
    “has_gp”—indicates whether a Google Play application is on the device (the true or false value);
    "Install from = " - "Google Play”/”Others”—indicates whether the infected application is installed from Google Play or from a third-party app store.

Once any of installed applications is launched, the Trojan sends a POST request that contains the above-mentioned information, adding the name of the running application’s package and requesting advertising parameters:

    "index" – requestads;
    “package_name_running”—name of the running application.

Depending on a received command, Android.Spy.277.origin can execute the following actions:

    “show_log”—enable or disable logging;
    “install_plugin”—install a plug-in hidden inside the malicious application;
    “banner”, “interstitial”, “video_ads”—display different types of advertisements (including, on top of the OS interface and other applications);
    “notification”—display a notification with the received parameters;
    “list_shortcut”—create shortcuts on the home screen (tapping these shortcuts leads to opening of specified sections in Google Play);
    “redirect_gp”—open a webpage with a specified address in Google Play;
    “redirect_browse”—open a specified webpage in a preinstalled browser;
    “redirect_chrome”—open a specified webpage in Chrome;
    “redirect_fb”—open a Facebook webpage specified by the command.
