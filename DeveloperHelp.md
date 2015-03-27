# Introduction #

If you are a new (or even a not-so-new) Ares developer then this guide will be useful for you. It contains various pointers to documentation and references to help you find your way through the three projects that make up this project.

# Getting in Touch #

In general the most useful thing to do when you want to develop on CS, CEL, and/or Ares is to stay in touch with us. The two most popular ways are:

  * The #CrystalSpace IRC channel at FreeNode
  * The crystal-develop mailing list: https://lists.sourceforge.net/lists/listinfo/crystal-develop

# Requirements #

Ares is based on the Crystal Space and CEL projects so you need to get the latest source of those projects. Since Ares is in constant development and also requires constant changes to CS and CEL as well we can't depend on stable CS and CEL releases. So you need to get the development versions (trunk) from svn using a subversion client:

```
svn co svn://svn.code.sf.net/p/crystal/code/CS/trunk CS
svn co svn://svn.code.sf.net/p/cel/code/cel/trunk cel
svn co http://ares.googlecode.com/svn/trunk/ ares-read-only
```

# Building #

  * Documentation included with CS project: http://www.crystalspace3d.org/docs/online/manual/Building.html
  * Documentation included with CEL project: http://crystalspace3d.org/cel/docs/online/manual/Building.html
  * Documentation and summary for building Ares: AresBuilding

# Overall Structure #

The [Architecture](Architecture.md) page on this wiki explains the overal structure of this project. It is recommended you start with this before reading the rest of the documentation below.

# Basics of Crystal Space #

Crystal Space is a big project but luckily you don't need to know everything to be able to develop with it. Depending on the task you are doing there are various modules you need to know better but at the very least you should read the following sections of the documentation to understand the basics of CS a bit better:

  * Basics. Crystal Space, CEL, and Ares are heavily based on plugins. The basic section in the documentation explains the plugin system used in CS. Also the section explaining basic Crystal Space concepts is important: http://www.crystalspace3d.org/docs/online/manual/The-Basics.html
  * Tutorials. The tutorial section is a nice way to get used to how basic Crystal Space programs are made. I recommend to at least skim this part of the documentation: http://www.crystalspace3d.org/docs/online/manual/Tutorials.html
  * VFS. The Virtual File System in CS is very important. I recommend you at least skim this chapter to get an idea of what it can do and how it is used: http://www.crystalspace3d.org/docs/online/manual/VFS.html


The following chapters are very useful but are more advanced and you can postpone reading them until you actually need something here:

  * Creating your own plugins and SCF. This is a bit more advanced information on how to use SCF and make your own plugins. This may come in handy at some point but to start with you can skip this section: http://www.crystalspace3d.org/docs/online/manual/SCF-Chapter.html
  * Event system. Crystal Space is heavily event based. This is also a more advanced section of the manual. Skip this until you know you need to work with events: http://www.crystalspace3d.org/docs/online/manual/Event-System.html
  * Using Crystal Space in general. The entire 'Using CS' chapter is very useful and you should browse through this to get an idea of what is there: http://www.crystalspace3d.org/docs/online/manual/Using-Crystal-Space.html

# Basics of CEL #

CEL is a game logic system on top of Crystal Space. Ares heavily uses CEL for game logic so it is important that you know a bit on CEL as well. Unfortunatelly the CEL manual is a bit outdated in some areas but most concepts are still useful:

  * To learn about the various concepts defined by CEL you should read this: http://crystalspace3d.org/cel/docs/online/manual/Concepts.html
  * More detailed information about most of the property classes can be found here: http://crystalspace3d.org/cel/docs/online/manual/Property-Classes.html

# Basics of Ares #

At TechDoc you can find some information about the basics of Ares. It is not 100% up-to-date but I hope to solve this soon.

There is also the built-in documentation which you can access using the 'F1' key from within AresEd (or pointing your browser to the documentation as it gets installed on your harddisk).