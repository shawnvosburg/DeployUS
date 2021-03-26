package main

import (
	"fmt"
	"encoding/json"
	"io/ioutil"
	"net/http"
	"os"
	"os/exec"
)

type watchable struct {
	File string `json:"file"`
	Name string `json:"name"`
}

func main() {
	http.HandleFunc("/up", dockerComposeUp)
	http.HandleFunc("/down", dockerComposeDown)

	// DeployUS is on port 5000, WatchUS is on port 5001
	// Following this logic, WorkUS will be on port 5002
	http.ListenAndServe(":5002", nil)
}

func dockerComposeUp(writer http.ResponseWriter, reqest *http.Request) {
	// Initialize empty toWatch
	var toWatch watchable

	// Parse request for the json which contains the file
	body, _ := ioutil.ReadAll(reqest.Body)
	errJSON := json.Unmarshal([]byte(body), &toWatch)
	if errJSON != nil {
		writer.WriteHeader(http.StatusInternalServerError)
		writer.Write([]byte("Couldn't understand the JSON body."))
		return
	}
	defer reqest.Body.Close()

	// Find the path that will host the docker-compose.yml
	parentDir := fmt.Sprintf("/work/%s",toWatch.Name)
	scriptPath := fmt.Sprintf("/work/%s/docker-compose.yml",toWatch.Name)

	// Make sure all parent directories exists
	_, err := os.Stat(parentDir)
	if os.IsNotExist(err) {
		mdkirErr := os.Mkdir(parentDir, 0700) //All permission to user
		if mdkirErr != nil {
			writer.WriteHeader(http.StatusInternalServerError)
			errString := fmt.Sprintf("Could not create the %s directory. %s", parentDir, mdkirErr)
			writer.Write([]byte(errString))
			return
		}
	}

	// Create and fill files with contents
	file, err := os.Create(scriptPath)
	file.WriteString(toWatch.File)
	if err != nil {
		writer.WriteHeader(http.StatusInternalServerError)
		errString := fmt.Sprintf("Could not create the %s.", scriptPath)
		writer.Write([]byte(errString))
		return
	}

	// With the docker-compose.yml file in the /work directory, we can now launch it!
	// Pull necessary images
	cmdArgs := []string{"-f", scriptPath, "pull", "--ignore-pull-failures"}
	_, err = exec.Command("docker-compose", cmdArgs...).Output()
	if err != nil {
		// Leaving this here as the --ignore-pull-failures still throws an error if the user is not logged in!
		// In this project, WorkUS does not require logging in as all images are public and pulled from docker hub.
		// If a required images is not pulled properly, it will be catched later with docker-compose up.
		//writer.WriteHeader(http.StatusInternalServerError)

		writer.Write([]byte("Could not pull the docker images."))

	}

	// Run in detach mode.
	cmdArgs = []string{"-f", scriptPath, "up", "-d", "--force-recreate"}
	_, err = exec.Command("docker-compose", cmdArgs...).Output()

	// Catch all errors, useful for CI
	if err != nil {
		writer.WriteHeader(http.StatusInternalServerError)
		writer.Write([]byte("Could not call docker-compose up!"))
		return
	}
}

func dockerComposeDown(writer http.ResponseWriter, reqest *http.Request) {
	// Initialize empty toWatch
	var toWatch watchable

	// Parse request for the json which contains the file
	body, _ := ioutil.ReadAll(reqest.Body)
	errJSON := json.Unmarshal([]byte(body), &toWatch)
	if errJSON != nil {
		writer.WriteHeader(http.StatusInternalServerError)
		writer.Write([]byte("Couldn't understand the JSON body."))
		return
	}
	defer reqest.Body.Close()

	// Obtain the path to the script
	scriptPath := fmt.Sprintf("/work/%s/docker-compose.yml",toWatch.Name)

	// Assumes the docker-compose.yml is in its /work directory
	// Run in detach mode.
	cmdArgs := []string{"-f", scriptPath, "down"}
	_, err := exec.Command("docker-compose", cmdArgs...).Output()

	// Catch all errors.
	if err != nil {
		writer.WriteHeader(http.StatusInternalServerError)
		writer.Write([]byte("Could not call docker-compose down!"))
		return
	}
}
