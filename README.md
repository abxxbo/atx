<p align="center"><img src="https://media.discordapp.net/attachments/905219914774741002/947699828526317588/d.png"/></p>
abxxbo's text editor

## Checklist

### Main
- [X] Save to file
- [X] Read file
- [X] Syntax highlighting
- [X] Search for keyword
- [ ] Line numbers

## Other
- [X] Config file for ATX
- [ ] Plugin system
- [ ] More syntax highlighting (desired: Make and Markdown)

# Screencast
![dh](img/screencast.gif)

# Screenshots
![dh](img/c-styl.png)
<p><em>ATX editing a C file with C style syntax highlighting</em></p>
<br>
<br>

![dh](img/py-styl.png)
<p><em>ATX editing a Python file with Python syntax highlighting</em></p>

## Dependencies
- make
- Any C compiler

## Compilation
```sh
# Compile atx
$ make
```

# Milestones
This is a list of all milestones that have been accomplished.

- [9a96e38 - read full file](https://github.com/abxxbo/atx/commit/9a96e38c33c19fe38f69ea7c1d6b289886c643c4)
- [c62184f - status bar](https://github.com/abxxbo/atx/commit/c62184f978dc3566cfc4189cb7242492905c3a3d)
- [9e069fc - write to a file](https://github.com/abxxbo/atx/commit/9e069fc73c8675074e67a6a5cc70267778939dca)

<br>

# Plugins

A stand alone program has been written to make and manage plugins for atx. Compiling is not needed, since
it's a bash script. You can install this by this command:
```sh
# or whereever you want to install it, not just  /usr/local/bin/
sudo cp src/plugin/atx-plugin /usr/local/bin/atx-plugin
```

## Checklist
- [ ] INI parser
- [ ] Download requests from URL passed into program

## Basic usage
Example plugins can be found in the plugins/ directory. You may install a plugin via:
```
atx-plugin [URL] [folder]
```

Instead of using a URL as your plugin, you can use a local directory. Like so:
```
atx-plugin folder/
```

### Updating plugins
TODO.

# Creating your own plugins
It's very easy to create plugins for atx. Every plugin must have these files:

- Metadata for the plugin, stored in `config.ini`
- A C header file, which is named as `[plugin_name].h`.

## Requirements for a valid plugin

### Metadata file
For a metadata file, only 3 fields are required: name, author, and version. You may write your metadata file like so:
```ini
[plugin]
name=Plugin Name
author=Author
version=1
```

### Plugin header file

#### Functions required
There are 2 functions required for a plugin: `on_init()` and `on_shutdown()`.

#### Example
```c
// An example plugin, written by abxxbo.
// All this plugin does is writes "Hello World" to
// /tmp/plug_out. you can view this by running
// tail -f /tmp/plug_out


void on_init(){
  FILE* fp = fopen("/tmp/plug_out", "a");
  fputs("Hello world", fp);
}

// we are not using on_shutdown() at all, so we can just
// define it as an empty function
void on_shutdown(){}
```

<br>

# How to configure ATX?

**WARNING!** The configuration system is very WIP, so do not use until this message is gone.

## Setting tab spacing
```sh
# Example: tabs 2
# Default is 2
tabs [tab spacing]
```

## Change empty buffer character
```sh
# Example: eob_char "."
# Default is "~"
eob_char "[character]"
```

# Is this a good replacement for X?
Depends on the editor

## Vim / Neovim
No.

## GNU Nano
In my opinion, yes.

## Emacs
No.

## Other editors
I am not listing every single editor, so this is up to you.
