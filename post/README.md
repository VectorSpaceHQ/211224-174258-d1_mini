
These tools are used to post-process the dust collection data.

The MQTT messages are captured into a sql database by mqtt-data-logger-sql.py which is run as a systemd service by installing and enabling dust_collection_logger.service. This is done on the mqtt server where it must run perpetually in order to capture the mqtt messages as they happen.

view_table.py gives a quick look at the last few entries of the database.

tool_use_plotter.py generates plot showing the usage of each tool.