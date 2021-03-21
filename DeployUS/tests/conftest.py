import pathlib
import requests
import pytest
import mysql.connector

INIT_SQL_PATH = pathlib.Path(__file__).parent.absolute().joinpath('../db/init.sql')


class DataBase():
    def __init__(self):
        config = {
            'user': 'root',
            'password': 'deployus',
            'host': '127.0.0.1',
            'port': '32000',
            'database': 'deployusdb'
        }
        self.connection = mysql.connector.connect(**config)
        self.cursor = self.connection.cursor()
    
    def query(self,query):
        # Send command. Must set argument if multiple querys (marked by semi-colon)
        multi = query.count(';') > 1
        self.cursor.execute(query, multi=multi)

        # Fetch results, if any
        results =  [ elem for elem in self.cursor]

        # Commit to the mysql connection
        self.connection.commit()

        # Return
        return results

    def close(self):
        # Clearing database for next test
        self.query("DELETE FROM jobs;")
        self.query("DELETE FROM scripts;")
        self.query("DELETE FROM workers;")

        # Assertions that all tables are empty
        assert len(self.query("SELECT * FROM jobs;")) == 0
        assert len(self.query("SELECT * FROM scripts;")) == 0
        assert len(self.query("SELECT * FROM workers;")) == 0
        
        # Part 0 specifics
        self.query("INSERT INTO workers (name, location) VALUES ('Localhost','127.0.0.1');")
        assert len(self.query("SELECT * FROM workers;")) == 1

        # Close connection
        self.connection.close()
        self.cursor.close()

class DeployUSInterface():
    def __init__(self):
        self.addr = 'http://127.0.0.1:5000'

    def index_page(self):
        return requests.get(self.addr + '/') 

    def workers_page(self):
        return requests.get(self.addr + '/workers') 

    def launch_page(self):
        return requests.get(self.addr + '/launch') 

    def get_scripts(self):
        return requests.get(self.addr + '/get_scripts') 

    def get_jobs(self):
        return requests.get(self.addr + '/get_jobs') 

    def get_workers(self):
        return requests.get(self.addr + '/get_workers') 

    def insert_script(self, script_name, script_file):
        payload = {'name': script_name}
        files = {'scriptfile': script_file}
        response = requests.post(self.addr + '/insert_script', data=payload, files=files)
        return response

    def delete_script(self, script_id):
        payload = {'id': script_id}
        response = requests.post(self.addr + '/delete_script', json=payload)
        return response

    def launch_job(self, script_id, location):
        payload = {'id': script_id, 'location':location}
        response = requests.post(self.addr + '/launch_job', json=payload)
        return response

    def stop_job(self, job_id):
        payload = {'id': job_id}
        response = requests.post(self.addr + '/stop_job', json=payload)
        return response

    def insert_worker(self, name, location):
        payload = {'name': name, 'location': location}
        response = requests.post(self.addr + '/insert_worker', json=payload)
        return response

    def delete_worker(self, worker_id):
        payload = {'id': worker_id}
        response = requests.post(self.addr + '/delete_worker', json=payload)
        return response


# Takes care of setup & teardown of database interface
@pytest.fixture()
def db():
    # Create DataBase object
    db = DataBase()

    # Perform test
    yield db

    # Teardown
    db.close()