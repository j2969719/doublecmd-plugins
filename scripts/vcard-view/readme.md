vCard view
----------

Simple viewing, text only.

Properties:
FN or N<br>
NICKNAME<br>
TEL<br>
EMAIL<br>
ADR<br>
URL<br>
ORG<br>
TITLE<br>
ROLE<br>
BDAY<br>
PHOTO (`yes` or `no`)<br>
CATEGORIES<br>
NOTE

## `vcard.py`

For [gtk_socket](plugins/wlx/gtk_socket) or [gtk_socket_tst](plugins/wlx/gtk_socket_tst) only.<br>
Add script to the `scripts` subfolder and add to `settings.ini`
```ini
[vcard]
script = vcard.py
[vcf]
redirect = vcard
```

## `vcard_print.py`

Faster, can be used in

[fileinfo](plugins/wlx/fileinfo)/[fileinfo_qt](plugins/wlx/fileinfo_qt) plugin: add to `fileinfo.sh`
```sh
	[Vv][Cc][Aa][Rr][Dd]|[Vv][Cc][Ff])
		$HOME/.local/share/doublecmd/scripts/vcard_print.py "$file"
		;;
```

[mimescript](plugins/wlx/mimescript) plugin: add to `settings.ini`
```ini
[text/vcard]
script = $HOME/.local/share/doublecmd/scripts/vcard_print.py
```
[internal file associations](https://github.io/doublecmd.github.io/doc/en/configuration.html#ConfigAssociations):
file types: for example, `vCard`<br>
extensions: `vcard|vcf`<br>
actions: `View`<br>
command: `{!DC-VIEWER}`<br>
params: `<?$HOME/.local/share/doublecmd/scripts/vcard_print.py %p0?>`

[toolbar button](https://github.io/doublecmd.github.io/doc/en/toolbar.html):
external command: `$HOME/.local/share/doublecmd/scripts/vcard_print.py`<br>
params: `%t1 %p0`

