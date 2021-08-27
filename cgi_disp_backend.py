#!C:\Users\rob\AppData\Local\Programs\Python\Python37\python.exe
import sqlite3
import os.path
import json
import cgi
#  This script is for gathering the JSON available at the following API, cleaning
#  the data and delivering it to JavaScript.

def getKWHData(dbPath, timeLength=0, showBy="h"):
    # Process the total KWH and return values by hour for a given time period
    # DEBUG - START WITH 1 WEEK
    #  date('now', 'localtime', 'weekday 0', '-6 days'); start of week?
    #   date('now', 'localtime', 'weekday 0'); end of week?

    labels = []
    data = []
    totalKWH = 0
    results = []

    sqlXinsert = "substr(time, 1, 2)"  # Default plot by hour
    sqlRangeInsert = "'2020-09-02'" #"date('now','localtime')"       # Default to today

    if (showBy=="h"):
        sqlXinsert = "substr(time, 1, 2)"
    if (showBy=="d"):
        sqlXinsert = "substr(date, 9, 2)"
    if (showBy=="m"):
        sqlXinsert = "substr(date, 6, 2)"


    sqlStatement = ("select " + sqlXinsert + ", round(avg(rpa+rpb)/1000, 2)  from readings where date = " 
                    + sqlRangeInsert + " group by " + sqlXinsert +";")

    sqlConn = sqlite3.connect(dbPath)
    sql = sqlConn.cursor()
    sql.execute(sqlStatement)
    #results = json.dumps(sql.fetchall())
    for row in sql:
        #result = row.split(",")
        labels.append(str(row[0]))
        data.append(float(row[1]))
        totalKWH += float(row[1])
    
    results.append(labels)
    results.append(data)
    results.append(totalKWH)
    
    sqlConn.close()   
    results = json.dumps(results)
    return(results)

def getVoltData(dbPath, timeLength=0, showBy="h"):
    labels = []
    dataVa = []
    dataIa = []
    dataVb = []
    dataIb = []
    results = []

    sqlXinsert = "substr(time, 1, 2)"  # Default plot by hour
    sqlRangeInsert = "'2020-09-02'" #"date('now','localtime')"       # Default to today

    # add showBy minute
    if (showBy=="h"):
        sqlXinsert = "substr(time, 1, 2)"
    if (showBy=="d"):
        sqlXinsert = "substr(date, 9, 2)"
    if (showBy=="m"):
        sqlXinsert = "substr(date, 6, 2)"


    sqlStatement = ("select " + sqlXinsert + ", round(avg(va), 2), round(avg(ia), 2), round(avg(vb), 2), round(avg(ib), 2) from readings where date = " 
                    + sqlRangeInsert + " group by " + sqlXinsert +";")

    sqlConn = sqlite3.connect(dbPath)
    sql = sqlConn.cursor()
    sql.execute(sqlStatement)
    #results = json.dumps(sql.fetchall())
    for row in sql:
        labels.append(str(row[0]))
        dataVa.append(float(row[1]))
        dataIa.append(float(row[2]))
        dataVb.append(float(row[3]))
        dataIb.append(float(row[4]))
    
    results.append(labels)
    results.append(dataVa)
    results.append(dataIa)
    results.append(dataVb)
    results.append(dataIb)
    
    sqlConn.close()   
    results = json.dumps(results)
    return(results)


def getPowerData(dbPath, timeLength=0):
    labels = []
    data = []
    results = []

    sqlConn = sqlite3.connect(dbPath)
    sql = sqlConn.cursor()
    sql.execute('SELECT * FROM readings LIMIT 12')
    #results = json.dumps(sql.fetchall())
    for row in sql:
        #result = row.split(",")
        labels.append(str(row[3]))
        data.append(float(row[4]))
    
    results.append(labels)
    results.append(data)
    
    sqlConn.close()   
    results = json.dumps(results)
    return(results)


def main():
  
    results = []

    # Grab the passed paramater from AJAX
    parameter = cgi.FieldStorage()
    queryType = parameter.getvalue("param")
  
    # Get the working DIR of the database, may be different in web instance from actual path
    baseDir = os.path.dirname(os.path.abspath(__file__))
    dbPath = os.path.join(baseDir, "whem.db")
    
    # What page is requesting? Pull and process the correct data
    if (queryType == "kwh"):
        results = getKWHData(dbPath)
    if (queryType == "volts"):
        results = getVoltData(dbPath)
    if (queryType == "power"):
        results = getPowerData(dbPath)
    
    # Return Data to display on page
    print("Content-type: application/json\n\n") #text/html\n\n")
    print(results)
    return (results)


if __name__ == '__main__':
    main()
