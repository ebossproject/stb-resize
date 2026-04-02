> ⚠️ **SECURITY & USE NOTICE**
> 
> Unless specified otherwise, the contents of this repository are distributed according to the following terms.
> 
> Copyright (c) 2024-2026 Cromulence LLC. All rights reserved.
> 
> This material is based upon work supported by the Defense Advanced Research Projects Agency (DARPA) under Contract No. HR001124C0487. Any opinions, finding and conclusions or recommendations expressed in this material are those of the author(s) and do not necessarily reflect the views of DARPA.
> 
> The U.S. Government has unlimited rights to this software under contract HR001124C0487.
>
> DISTRIBUTION STATEMENT A. Unlimited Distribution
>
> Original work Licensed under MIT License (See `LICENSE`). Derived from open source libraries under respective licenses - see (`THIRD_PARTY_LICENSES.md`)
>
> This repository is provided **solely for research, educational, and defensive security analysis purposes**.
>
> The software contained herein **intentionally includes known vulnerabilities** in third-party libraries and application logic. These vulnerabilities are included **for the purpose of studying reachability analysis, exploitability, and remediation techniques**.
>
> **DO NOT deploy this software in production environments**, on internet-accessible systems, or on systems containing sensitive data.
>
> By downloading, building, or running this software, you acknowledge that you **understand its intended purpose and potential security impact**, and that you assume all responsibility for its use.

# stb-resize
In this challenge, a vulnerability exists in one of the two vcpkg managed dependencies - stb and libyaml. Using the PoV, users can generate a coredump to identify the root cause of the vulnerability and then remediate or reproduce the triggering input.

## Input Surface
The challenge acts as a server to listen for connections, commands, and data to process images based on a yaml configuration. The incoming network is the primary attack surface. The server accepts a yaml file that specifies the operation the server will take as input and the png file to be resized also is part of the attack surface.

## Usage
To build locally, use the following commands:

To build the dockerfiles, run the following command `docker compose build`.

To begin running the server, run the following command `docker compose up -d server`

To test stb-resize with the poller tool, run `docker compose run --rm poller`

To test stb-resize with your modified PoV, run `docker compose run --rm pov`

To stop any currently running containers, run `docker compose down -v`

## Build
This challenge structure relies on vcpkg package manager and cmake working coopreratively to build stb-resize. This challenge is developed primarily to be built using the Dockerfile locally with Compose.

In this form, cmake will create the build directory, chainload the custom toolchain file to install the dependencies, and build for Release according to the target triplet.

The build leverages the following environmental variables from the build image:

- `VCPKG_OVERLAY_TRIPLETS` - specifies directory to look for target triplets
- `VCPKG_OVERLAY_PORTS` - specifies location to search ports (takes precedence over vcpkg repo)
- `TOOLCHAIN_FILE` - specifies toolchain path and file
- `TRIPLET` - specifies the triplet to use for the cmake configuration
- `VCPKG_ROOT` - specifies the root of the vcpkg install location and other contents

## Poller
The poller runs tests of the stb-resize software to ensure basic functionality. All tests are expected to pass. This can be used to determine if build modifications have affected the behavior of the system or not. The poller can be run in a docker container as described above or locally by running the shell script provided in the poller directory.

## Input Synthesis
To demonstrate the ability to effectively recover a trigger from a crash dump, users may elect to reproduce the PoV as part of the Input Synthesis workflow. This repository includes a `/pov` directory with the necessary Dockerfile and `pov.py` script to provide inputs to the system. The pov is currently prepared to generate a crashing input already. Users may modify this to substitute their own POC.

## Remediation
This server has no need to process invalid arguments or files. Remediation should be achieved through bypassing the malformed inputs and allowing the server to continue processing incoming messages without crashing. The PoV template script does not necessarily expect a response from the server if a malformed input is ignored.
