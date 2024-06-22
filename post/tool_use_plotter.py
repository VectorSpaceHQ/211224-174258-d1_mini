#!/usr/bin/env python3

import sqlite3 as lite
from collections import defaultdict
import matplotlib.pyplot as plt
import numpy as np
from datetime import datetime
import matplotlib.dates as mdates



# TOOL_NAME = tools[1]

# initializing dict with lists
conn = lite.connect('./dust_collection_logs.db')
cur = conn.cursor()

tool_dict = defaultdict(list)

def get_logs():
    """
    Pulls logs for all tools
    """
    print("Pulling logs from database...")
    cur.execute("SELECT * FROM logs")
    lines = cur.fetchall()
    for line in lines:
        timestamp = line[1]
        tool_name = line[3]
        state = line[4]
        tool_dict[tool_name].append([timestamp,state])
    print("Finished retrieving logs.")

def plot_tool_use(ndays, tool_name):
    print("Generating plot for "+tool_name)
    now = datetime.now()
    timestamps = []
    states = []
    duration = 0
    durations = []
    duration_timestamps = []
    first_date = ''

    for i,x in enumerate(tool_dict[tool_name]): 
        time = x[0]
        state = x[1].strip(" ")

        idx = tool_dict[tool_name].index([x[0], x[1]]) # need the actual index if the list isn't traversed in order
        time = datetime.strptime(time, '%Y-%m-%d %H:%M:%S')

        # if older than ndays, skip
        if (now - time).days > ndays:
            continue
            
        if first_date == '':
            first_date = time

        if (state != "OFF" and state != "ON"): # skip junk
            continue
        if idx +1 >= len(tool_dict[tool_name]):
            break

        if state == "ON":
            state = 1
        else:
            state = 0

        timestamps.append(time)
        states.append(state)

    appended_timestamps = []
    appended_states = []
    for i, state in enumerate(states[:-1]):
        # Insert second ON point just before OFF to avoid sawtooth plot. Same for OFF.
        # ie. insert current state at the next time.
        # ON 2023-07-10 18:20:35
        # ON 2023-07-10 18:22:07  <---- inserted
        # OFF 2023-07-10 18:22:07
        # OFF 2023-07-11 08:11:11 <---- inserted
        # ON 2023-07-11 08:11:11
        # next_time = tool_dict[TOOL_NAME][idx+1][0]
        next_state = states[i+1]
        time = timestamps[i]
        next_time = timestamps[i+1]

        appended_timestamps.append(time)
        appended_states.append(state)

        if next_state != state:
            appended_timestamps.append(next_time)
            appended_states.append(state)

        # Integrate time on
        if state == 1:
            # insert old
            duration_timestamps.append(time)
            durations.append(duration)
            
            duration_timestamps.append(next_time)
            duration += (next_time - time).total_seconds() / 3600
            durations.append(duration)
        # print(state, time, next_time, duration)
        #--------------------

    # for i in range(len(appended_timestamps)):
    #     print(appended_states[i], appended_timestamps[i])



    # left = datetime.strptime(timestamps[0], '%Y-%m-%d %H:%M:%S')
    # right = datetime.strptime(timestamps[-1], '%Y-%m-%d %H:%M:%S')
    print("Since {}, the {} has run for {} hours.".format(first_date, tool_name, round(duration,1)))

    # debug
    # print(len(states), len(duration_timestamps), len(durations))
    # for i in range(len(durations)):
    #     print(states[i],duration_timestamps[i], durations[i])
    # for i in range(len(timestamps)):
    #     print(states[i],timestamps[i])

    fig = plt.figure()
    ax1 = fig.add_subplot(1, 1, 1)
    fig.autofmt_xdate(bottom=0.2, rotation=30, ha='right')
    ax1.plot(appended_timestamps, appended_states, color='tab:blue')
    ax1.set_ylabel("State")
    ax1.set_yticks([0, 1])
    ax1.set_yticklabels(["OFF","ON"])
    ax1.set_xlabel("Date")
    ax1.set_title(tool_name)
    ax2 = ax1.twinx()  
    ax2.plot(duration_timestamps, durations, color='red')
    ax2.set_ylim(0);
    ax2.set_ylabel("Time ON (hrs)")
    ax2.xaxis.set_tick_params(rotation = 30)
    # Format the date into months & days
    ax2.xaxis.set_major_formatter(mdates.DateFormatter('%m-%d')) 
    
    # Change the tick interval
    # plt.gca().xaxis.set_major_locator(mdates.DayLocator(interval=30))
    ax2.xaxis.set_major_locator(mdates.DayLocator(interval=ndays//10)) 

    # Puts x-axis labels on an angle
    # Changes x-axis range
    # plt.gca().set_xbound(left, right)
    today = datetime.now().strftime("%Y-%m-%d")
    fig.savefig("./plots/"+tool_name+"_tool_use_"+today+".png")
    print("Finished generating plot for "+tool_name+"\n")

    
def main():
    ndays = 30
    tools = ['blue-bandsaw','Planer','Router Table','Miter Saw',
             "Table Saw (lathes)", "Dust Collector", "Edge Sander",
             "Green Bandsaw"]

    get_logs()
    
    for tool in tools:
        plot_tool_use(ndays, tool)


if __name__ == "__main__":
    main()
