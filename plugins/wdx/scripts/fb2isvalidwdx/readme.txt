fb2isvalidwdx.lua (Unix-like only)
2019.11.07

Validating FictionBook2 (FB2) files (boolean, returns true or false).

Requeres XMLStarlet (http://xmlstar.sourceforge.net) in PATH: find package "xmlstarlet" in your distribution or compile it.

NOTE 1: You can use first test (i.e. "well-formedness only") with FB2 and ANY others XML-based files.

NOTE 2: Validating against XSD schema for FB2-files only!
NOTE 2-a: Standard FictionBook2.2 is actual and recommended format of FB2-books.
NOTE 2-b: In some cases invalid FB2-files just contents non-standard genre(s).

P.S. In most cases, invalid files fixing is simple, you can find specific error (description and line of file), use:
  well-formedness only:
    xmlstarlet val --well-formed --err <file>
  with XSD schema:
    xmlstarlet val --xsd <xsd-file> --err <file>
