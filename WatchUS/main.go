package main

import (
	"fmt"
	"net/http"
	"os/exec"
)

func main() {
	http.HandleFunc("/up", dockerComposeUp)
	http.HandleFunc("/down", dockerComposeDown)

	// DeployUS is on port 5000, WatchUS will be on port 5001
	http.ListenAndServe(":5001", nil)
}

func dockerComposeUp(writer http.ResponseWriter, reqest *http.Request) {
	// Assumes the docker-compose.yml is in its /work directory
	// Pull necessary images
	cmdArgs := []string{"-f", "/work/docker-compose.yml", "pull"}
	_, err := exec.Command("docker-compose", cmdArgs...).Output()

	// Run in detach mode.
	cmdArgs = []string{"-f", "/work/docker-compose.yml", "up", "-d", "--force-recreate"}
	_, err = exec.Command("docker-compose", cmdArgs...).Output()

	// Catch all errors, useful for CI
	if err != nil {
		fmt.Printf("%s", err)
	}
}

func dockerComposeDown(writer http.ResponseWriter, reqest *http.Request) {

	// Assumes the docker-compose.yml is in its /work directory
	// Run in detach mode.
	cmdArgs := []string{"-f", "/work/docker-compose.yml", "down"}
	_, err := exec.Command("docker-compose", cmdArgs...).Output()

	// Catch all errors.
	if err != nil {
		fmt.Printf("%s", err)
	}
}