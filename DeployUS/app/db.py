import mysql.connector
import os
from app import utils

def mysql_safe(func):
    """
        Decorator to safely connect/disconnect from database
    """
    def inner(*args, **kwargs):
        config = {
        'user': 'root',
        'password': 'deployus',
        'host': 'db',
        'port': '3306',
        'database': 'deployusdb'
        }
        connection = mysql.connector.connect(**config)
        cursor = connection.cursor()

        # Execute function
        result = func(cursor, connection, *args, **kwargs)

        cursor.close()
        connection.close()

        return result
    return inner


# ========================================================
#   Interface with database
# ========================================================

@mysql_safe
def get_script(cursor=None, connection=None):
    cursor.execute('SELECT * FROM scripts')
    results = [(id, name, utils.formatDateTimeObj(str(date)), utils.getHash(contents)) for (id, name, date, contents) in cursor]

    return results

@mysql_safe
def get_jobs(cursor=None, connection=None):
    sql = f"""
        SELECT j.id, s.name, w.location
        FROM jobs AS j
        JOIN scripts AS s ON j.script_id = s.id
        JOIN workers AS w on j.worker_id = w.id;
    """
    cursor.execute(sql)
    results = [(id, script_name, worker_location) for (id, script_name, worker_location) in cursor]

    return results

@mysql_safe
def get_workers(cursor=None, connection=None):
    cursor.execute('SELECT * FROM workers')
    results = [(id, name, location) for (id, name, location) in cursor]

    return results

@mysql_safe
def insert_worker(cursor, connection, name, location):
    sql = 'INSERT INTO workers (name,location) VALUES (%s, %s);'
    val = (name, location)
    cursor.execute(sql,val)
    connection.commit()

@mysql_safe
def insert_script(cursor, connection, name_, contents):
    sql = 'INSERT INTO scripts (name, cre_date, contents) VALUES (%s, %s, %s );'
    val = (name_, utils.getDatetimeNow() , contents)
    cursor.execute(sql,val)
    connection.commit()
    
@mysql_safe
def delete_worker(cursor, connection, id):
    sql = f'DELETE FROM workers WHERE id = {id};'
    cursor.execute(sql)
    connection.commit()

@mysql_safe
def delete_script(cursor, connection, id):
    sql = f'DELETE FROM scripts WHERE id = {id};'
    cursor.execute(sql)
    connection.commit()

@mysql_safe
def launch_job(cursor, connection, id, location):
    cursor.execute(f"SELECT * FROM scripts WHERE id = '{id}'")
    results = [(id, name, str(date), contents) for (id, name, date, contents) in cursor]

    # Script does not exists
    if len(results) == 0:
        return
    (id, name, date, contents) = results[0]

    # Write file to disk
    parentdir = f"/work/scripts/{name}"
    dockercomposePath = os.path.join(parentdir,"docker-compose.yml")
    if not os.path.exists(parentdir):
        os.mkdir(parentdir)

    with open(dockercomposePath, 'wb') as file:
        file.write(contents)

    # docker compose execution
    cmd = f"cd {parentdir};  docker-compose up -d"
    os.system(cmd)

    # Insert job's location into jobs table in db
    sql = 'INSERT INTO jobs (script_id, worker_id) VALUES (%s, %s );'
    val = (id, 1 )
    cursor.execute(sql,val)
    connection.commit()


@mysql_safe
def stop_job(cursor, connection, job_id):
    sql = f"""
        SELECT j.id, s.name, w.location
        FROM jobs AS j
        JOIN scripts AS s ON j.script_id = s.id
        JOIN workers AS w on j.worker_id = w.id
        WHERE j.id = {job_id};
    """
    cursor.execute(sql)
    results = [(id, script_name, worker_location) for (id, script_name, worker_location) in cursor]

    # Script does not exists
    if len(results) == 0:
        return
    (id, script_name, worker_location) = results[0]

    # docker compose execution
    parentdir = f"/work/scripts/{script_name}"
    cmd = f"cd {parentdir};  docker-compose down"
    os.system(cmd)

    # Insert job's location into jobs table in db
    sql = f'DELETE FROM jobs WHERE id = {job_id}'
    cursor.execute(sql)
    connection.commit()

