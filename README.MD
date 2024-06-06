![Logo](https://upload.wikimedia.org/wikipedia/commons/f/f4/Logo_EPFL.svg)

# CS202 Computer Systems Project : ImgFS

## Introduction and motivation
The aim of this project was for us to develop a large program in C on a "system" theme. The framework chosen this year was the construction of a command-line utility to manage images in a specific format file system, inspired by the one used by Facebook (Meta).   
Facebook's system is called **Haystack** and is described in the following [paper](https://www.usenix.org/event/osdi10/tech/full_papers/Beaver.pdf) (The version implemented in this project is obviously a simplified one).

In the **Haystack** approach, several images are contained in a single file. What's more, different resolutions of the same image are stored automatically. This single file contains both data (the images) and metadata (information about each image). The key idea is that the image server has a copy of this metadata in memory, to enable very rapid access to a specific photo in the right resolution.

This project features both a command line interface as well as a (very basic) browser interface (provided through a server running on localhost).

## How to download and set up the project
DISCLAIMER : This project was designed to run on a UNIX system, it will NOT work on a MacOS or Windows machine.

1. Clone the repository to your machine

HTTPS :
```shell
git clone https://github.com/projprogsys-epfl/cs202-24-prj-sysbros.git
```
SSH :
```shell
git clone git@github.com:projprogsys-epfl/cs202-24-prj-sysbros.git
``` 

2. Navigate to the `done` directory
```shell
cd done
```

3. Compile the project (`-s` for a less verbose output).
```shell
make -s
```

4. You can now use it !

## How to run the program in the CLI (Command Line Interface)
This section supposes the program compiled succesfuly
- Get help
```shell
./imgfscmd help
```
- List the contents of an Imgfs :
```shell
./imgfscmd list <imgfs_name>
```
- Create an Imgfs :
```shell
./imgfscmd create <imgfs_name> [options]
```
- Read an image from an Imgfs and save it to a file :
```shell
./imgfscmd read <imgfs_name> <imgID> [resolution]
```
- Insert a new image in the Imgfs :
```shell
./imgfscmd insert <imgfs_name> <imgID> <file_name>
```
- Delete an image from the Imgfs :
```shell
./imgfscmd delete <imgfs_name> <imgID>
```
Refer to the help command for more information about existing options and resolutions (default ones will be used if not provided).

## How to run the program in your browser
This section supposes the program compiled succesfuly

1. Start the server with an existing Imgfs (default port is `8000`)
```shell
./imgfs_server <imgfs_name> [port]
```
2. In your browser, go to http://localhost:8000
3. That's it ! You can click on the images to view them (`read`), on the red cross to delete them (`delete`), or upload a new picture (`insert`)

Note : There are some already existing Imgfs that you can use instead of having to create one of your own for this section.  
You can find them under `done/tests/data`, they are the files that end with .imgfs (`test02.imgfs` to `test24.imgfs`, `full.imgfs`)

Beware that modifications made to an ImgFS are not reversible. Starting another server with the same ImgFS will not recreate the ImgFS that was provided with the clone.

## Dependencies
In order for the program to run properly, you will need the following libraries :
- `libssl` (used to compute the hash of some images)
```shell
sudo apt install libssl-dev
```
- `vips` (used to resize the images, this project supports version `8.12.1` and `8.15.1`)
```shell
sudo apt install libvips-dev
```
- `libjson`(used to serialize/deserialize data sent over http)
```shell
sudo apt install libjson-c-dev
```