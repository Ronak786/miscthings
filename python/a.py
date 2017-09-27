"""
conn = mysql.connect
conn.execute("select keyword, info from fishseameta where keyword in (select keyword from baidumeta))
get file list from conn's result
for every keyword, info  in list
    if unique
        make a dir named keyword
        in that dir make a file 
        find in namemap engname, write name into that file
        store info into file, append

        select saveURL from googleemta where keyword=engname and isfish=2
        select saveURL from baiduemta where keyword=chiname isfish=2
        var = picdir in baidumeta
        var2 = picdir2 in googlemeta
        for pic in var, var2:
        os.system("cp  pic dir")
"""

import pymysql
import os
import sys

conn = pymysql.connect(host="127.0.0.1", password="123456", user="root", database="fishsdb", use_unicode=True, charset="utf8")
cur = conn.cursor()

#this can be inverse, search baidu in fishsea or search fishsea in baidu, which is better?
cur.execute("select keyword, info from fishseameta where keyword in (select keyword from baidumeta))")
reslist = cur.fetchall()
nameset = set()
basedir=""
origdir=""
for res in reslist:
    if res[0] in nameset:
        print("already processed {}".format(res[0]))
        continue
    nameset.add(res[0])
    picdir = os.path.join(basedir, res[0])
    if not os.path.exists(picdir):
        os.makedirs(picdir)
    #write info file
    cur.execute("select engname from namemap where chiname = %s", (res[0],))
    try:
        engname = cur.fetchone()[0]
    except IndexError:
        print ("no engname of chinaem {}".format(res[0]))
        continue
    with open(os.join(picdir, "info"), "w") as f:
        f.write(engname)
        f.write(res[1])

    conn.execute("select saveURL from googlemeta where keyword = %s and isfish=2", (engname,))
    resgoogle = conn.fetchall()
    conn.execute("select saveURL from baidumeta where keyword = %s and isfish=2", (chiname,))
    resbaidu = conn.fetchall()
    for url in resgoogle + resbaidu:
        picfile = os.join(origdir, url[0])
        destfile = os.join(picdir, url[0])
        with open(picfile, "rb") as f:
            with open(destfile, "wb") as g:
                g.write(f.read())
    conn.commit()
conn.commit()
cur.close()
conn.close()


