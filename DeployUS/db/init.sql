CREATE DATABASE deployusdb;
use deployusdb;

CREATE TABLE scripts (
  id INT NOT NULL AUTO_INCREMENT PRIMARY KEY,
  name VARCHAR(100) NOT NULL,
  cre_date DATETIME NOT NULL,
  contents BLOB NOT NULL,
  UNIQUE (name)
);

CREATE TABLE workers (
  id INT NOT NULL AUTO_INCREMENT PRIMARY KEY,
  name VARCHAR(100) NOT NULL,
  location VARCHAR(100) NOT NULL,
  UNIQUE (name),
  UNIQUE (location)
);

CREATE TABLE jobs (
  id INT NOT NULL AUTO_INCREMENT PRIMARY KEY,
  script_id INT NOT NULL,
  worker_id INT NOT NULL,
  launch_date DATETIME NOT NULL,
  FOREIGN KEY (script_id) REFERENCES scripts(id),
  FOREIGN KEY (worker_id) REFERENCES workers(id),
  UNIQUE (script_id, worker_id),
  UNIQUE (worker_id)
);