# CO-squad

CO-squad is a smart container monitoring system developed for the Creativa Hackathon. It uses an ESP8266 microcontroller to monitor multiple containers for temperature, humidity, and distance using various sensors, and sends email alerts when containers are full. It features a web server interface for easy monitoring and management of container parameters.

## Table of Contents
- [Introduction](#introduction)
- [Features](#features)
- [Installation](#installation)
- [Usage](#usage)
- [Code Explanation](#code-explanation)
- [Contributing](#contributing)
- [License](#license)

## Introduction

CO-squad is designed to help monitor containers that hold specific materials or liquids, utilizing ultrasonic sensors for measuring the container's fill level and DHT sensors for temperature and humidity. The system sends email alerts when containers are full, and it provides a web interface to manage container settings such as capacity and height.

## Features

- **Real-time Monitoring**: Displays temperature, humidity, and distance (fill level) for each container.
- **Email Alerts**: Sends email notifications when a container is full.
- **Web Interface**: Easily update container parameters such as capacity and height, and add new containers.
- **Multiple Containers**: Supports monitoring of multiple containers with customizable parameters.

## Installation

Follow these steps to install and set up the CO-squad system on your ESP8266:

### Step 1: Clone the Repository
```bash
git clone https://github.com/your-username/CO-squad.git
cd CO-squad
