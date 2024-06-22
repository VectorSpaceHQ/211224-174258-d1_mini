#!/usr/bin/env python3
# Print last N lines from the dust collection log

import sqlite3 as lite

N = 350

conn = lite.connect('./dust_collection_logs.db')
cur = conn.cursor()

def get_posts():
    cur.execute("SELECT * FROM (SELECT * FROM logs ORDER BY id DESC LIMIT " + str(N) + ") AS sub ORDER BY id ASC")
    lines = cur.fetchall()
    for line in lines:
        print(line)

get_posts()
