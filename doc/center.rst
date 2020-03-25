------
center
------

================================================
center a text file & hide the terminal interface
================================================

:date: March 2020
:version: 0.0
:manual section: 1
:manual group: General Commands Manual

synopsis
--------
center `file`

description
-----------
center centers a text file in the terminal and hide the user interface while doing so (prompt, cursor etc.)

it also handles resizing events to keep the text centered while active

example
-------
::

    $ center <(cal)
    $ center ideal.txt
    $ cat spleen.txt | center
