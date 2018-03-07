# TTLDetector
![Plugin image](https://github.com/kensoSolutions/TTLDetector/blob/readme-edit/plugin.jpg "Plugin image")  
TTL detector plugin for the open-ephys [plugin-GUI](https://github.com/open-ephys/plugin-GUI/ "pluguin-GUI"). It detects TTLs in continuous data and packages them as events that can be read by downstream processors.
To initialize the TTL detector, you must configure the electrode layout. Here are the steps to follow:  
1. Select the type of electrode from the drop-down menu (you can have multiple types of electrodes in the same configuration).
2. Use the arrow buttons to select the number of electrodes of that particular type.
3. Hit the plus button to add those electrodes to the configuration.
4. Select the name of the electrode from the lower drop-down menu.
5. Change the settings for that electrode: (a) Update the name by typing in the drop-down menu, (b) inactivate individual channels by deselecting them to the right of the drop-down menu, or (c) change the channel mapping by clicking the "EDIT" button and opening the "channel drawer" .
6. Individual electrodes can be removed by clicking the "DELETE" button. The "MONITOR" button sends the channels of that electrode to the audio monitor.
## Installation
Copy the TTLDetector folder to the plugin folder of your GUI. Then build the plugin as described in the [wiki](https://open-ephys.atlassian.net/wiki/spaces/OEW/pages/491544/Installation "wiki").
