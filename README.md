_NOTE: This project is currently EXPERIMENTAL (as indicated by its
0.x.x version) and is being actively developed.  We offer no guarantee
of API stablility from release to release at this time._

### flux-core

flux-core implements the communication layer, lowest level
services and interfaces for the Flux resource manager framework.

#### Building flux-core

flux-core requires the following packages to build:
```
zeromq4-devel >= 4.0.4   # built --with-libsodium
czmq-devel >= 2.2.0
munge-devel
json-c-devel
```
Spec files for building zeromq4 and czmq packages on a RHEL 6 based
system are provided for your convenience in foreign/rpm.

If you want to build the MPI-based test programs, make sure that
`mpicc` is in your PATH before you run configure.  These programs are
not built if configure does not find MPI.

```
./autogen.sh   # skip if building from a release tarball
./configure
make
make check
```
#### Bootstrapping a Flux comms session

A Flux comms session can be started for testing as follows.
First, ensure that your MUNGE deamon is running, and
generate a set of (personal) CURVE keys for session security:
```
cd src/cmd
./flux keygen
Saving $HOME/.flux/curve/client
Saving $HOME/.flux/curve/client_private
Saving $HOME/.flux/curve/server
Saving $HOME/.flux/curve/server_private
```
No administrator privilege is required to start a Flux comms
session as described below.

##### Single rank session

To start a single-rank (size = 1) Flux comms session:
```
cd src/cmd
./flux start-single
```
A shell is spawned that has its environment set up so that Flux
commands can find the message broker socket.  When the shell exits,
the session exits.  Log output will be written to stderr.

##### Screen session

To start a Flux comms session (size = 8) using screen:
```
cd src/cmd
./flux start-screen -N8
```
Each broker instance runs in its own screen window.
Rank 0 will spawn a shell.   Connect to it by attaching
to window 0 of the screen session:
```
./flux start-screen --attach 0
```
or detach with
```
ctrl-a d
```
Exiting the rank 0 shell will terminate the session, but will
leave screen running.  To terminate the screen session and the Flux
session if it is still running:
```
./flux start-screen --shutdown
```
Log output will be written to the file `cmbd.log`.

##### SLURM session

To start a Flux comms session (size = 64) on a cluster using SLURM:
```
cd src/cmd
./flux start-srun -N 64
```
The srun --pty option is used to connect to the rank 0 shell.
When you exit this shell, the session terminates.
Log output will be written to the file `cmbd.log`.

#### Flux commands

To view the available Flux commands:
```
cd src/cmd
./flux -h
```