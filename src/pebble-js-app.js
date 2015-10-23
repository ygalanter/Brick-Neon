var version = '1.00';

// Listen for when the watchface is opened
Pebble.addEventListener('ready', 
  function(e) {
       //console.log ("++++ I am inside of 'Pebble.addEventListener(Ready));
  }
);


/*    ******************************************************************** Config Section ****************************************** */ 

Pebble.addEventListener("showConfiguration",
  function(e) {
   
    //Load the remote config page
    Pebble.openURL("http://codecorner.galanter.net/pebble/brick_neon_config.htm?version=" + version);
    
  }
);

Pebble.addEventListener("webviewclosed",
  function(e) {
    
    if (e.response !== '') {
      
      //console.log ("++++ I am inside of 'Pebble.addEventListener(webviewclosed). Resonse from WebView: " + decodeURIComponent(e.response));
      
      //Get JSON dictionary
      var settings = JSON.parse(decodeURIComponent(e.response));
   
      //Send to Pebble
      var app_message_json = {};

      // preparing app message
      app_message_json.KEY_HOURS_MINUTES_SEPARATOR = settings.hoursMinutesSeparator;
      app_message_json.KEY_DATE_FORMAT = settings.dateFormat;
      app_message_json.KEY_BLUETOOTH_BUZZ = settings.bluetoothBuzz;
      app_message_json.KEY_LANGUAGE = settings.language;
     
      
      //console.log ("++++ I am inside of 'Pebble.addEventListener(webviewclosed). About to send settings to the phone");
      Pebble.sendAppMessage(app_message_json,
        function(e) {
          //console.log ("++++ I am inside of 'Pebble.addEventListener(webviewclosed) callback' Data sent to phone successfully!");
        },
        function(e) {
          //console.log ("++++ I am inside of 'Pebble.addEventListener(webviewclosed) callback' Data sent to phone failed!");
        }
      );
    }
  }
);