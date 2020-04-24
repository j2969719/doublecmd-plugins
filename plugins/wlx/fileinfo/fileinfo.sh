#!/bin/bash

file=$1
filetype="${1##*.}"

case "${filetype}" in
	7[Zz]|[Ii][Mm][Gg]|[Ii][Mm][Aa]|[Vv][Mm][Dd][Kk])
		7za l "$file" 2>/dev/null || 7z l "$file"
		;;
	[Aa][Cc][Ee])
		file -b "$file" && unace v -y "$file" </dev/random
		;;
	[Aa][Ll][Zz])
		file -b "$file" && unalz -l "$file"
		;;
	[Aa][Nn][Ss])
		strings | cat "$file" |  iconv -f "866" -t "UTF-8"
		;;
	[Aa][Pp][Kk])
		aapt dump badging "$file"
		;;
	[Aa][Rr][Cc])
		file -b "$file" && arc l "$file"
		;;
	[Aa][Rr][Cc])
		file -b "$file" && arc l "$file"
		;;
	[Aa][Rr][Jj])
		file -b "$file" && arj l "$file" 2>/dev/null || \
		unarj l "$file"
		;;
	[Cc][Aa][Bb])
		file -b "$file" && cabextract -l "$file"
		;;
	[Cc][Pp][Ii][Oo])
		file -b "$file" && cpio -itv < "$file" 2>/dev/null
		;;
	[Cc][Rr][Tt]|[Cc][Ee][Rr]|[Pp][Ee][Mm])
		keytool -printcert -v -file "$file" || \
		openssl crl2pkcs7 -nocrl -certfile "$file" | openssl pkcs7 -print_certs -text
		;;
	[Cc][Ss][Vv])
		cat "$file" | sed -e 's/,,/, ,/g' | column -s, -t
		;;
	[Dd][Ee][Bb])
		dpkg-deb -I "$file" && echo && dpkg-deb -c "$file"
		;;
	[Dd][Jj][Vv][Uu]|[Dd][Jj][Vv])
		djvused -e print-pure-txt -u "$file"
		;;
	[Dd][Oo][Cc][Xx])
		docx2txt "$file" - || \
		unzip -p "$file" | grep --text '<w:r' | sed 's/<w:p[^<\/]*>/ \
			/g' | sed 's/<[^<]*>//g' | grep -v '^[[:space:]]*$' | sed G
		;;
	[Dd][Oo][Cc]|[Rr][Tt][Ff])
		catdoc -w "$file"
		;;
	[Dd][Vv][Ii])
		which dvi2tty >/dev/null 2>&1 && \
			dvi2tty "$file" || \
		catdvi "$file"
		;;
	[Ee][Pp][Uu][Bb])
		einfo -v "$file"
		;;
	[Ff][Bb][2])
		ebook2text "$file" || \
		xsltproc $COMMANDER_PATH/utils/fb2/FB2_2_txt_ru.xsl "$file" | iconv -f "windows-1251" -t "UTF-8"
		;;
	[Gg][Zz]|[Xx][Zz]|[Bb][Zz]2|[Ll][Zz][Mm][Aa]|[Tt][Xx][Zz]|[Tt][Gg][Zz]|[Tt][Bb][Zz]|[Tt][Ll][Zz])
		file -b "$file" && tar -tvf "$file" || zcat "$file" ||\
			xzcat "$file" || bzcat "$file"
		;;
	[Hh][Aa])
		file -b "$file" && ha lf "$file"
		;;
	[Hh][Tt][Mm][Ll]|[Hh][Tt][Mm])
		links -dump "$file" 2>/dev/null || w3m -dump "$file" 2>/dev/null || lynx -dump "$file"
		;;
	[Ii][Ss][Oo])
		isoinfo -d -i "$file" && isoinfo -l -R -J -i "$file"
		;;
	[Ll][Hh][Aa])
		file -b "$file" && lha l "$file"
		;;
	[Ll][Zz]4)
		file -b "$file" && lz4 -dc "$file" 2>/dev/null | tar -tvvf - || lz4 -dc "$file" 2>/dev/null | cat
		;;
	[Mm][Oo])
		file -b "$file" && msgunfmt "$file" || cat "$file"
		;;
	[Mm][Ss][Ii])
		file -b "$file" && msiinfo suminfo "$file" && msiextract -l "$file"
		;;
	[Oo][Dd][Tt])
		odt2txt "$file"
		;;
	[Pp][Dd][Ff])
		pdftotext -layout -nopgbrk "$file" -
		;;
	[Pp][Ss])
		ps2ascii "$file"
		;;
	[Rr][Aa][Rr])
		file -b "$file" && rar v -c- "$file" 2>/dev/null || \
		unrar v -c- "$file"
		;;
	[Tt][Aa][Rr])
		file -b "$file" && tar tvvf - < "${file}"
		;;
	[Tt][Oo][Rr][Rr][Ee][Nn][Tt])
		transmission-show "$file" || ctorrent -x "$file"
		;;
	[Xx][Ll][Ss])
		which xlhtml >/dev/null 2>&1 && {
			tmp=`mktemp -d ${TMPDIR:-/tmp}/%p.XXXXXX`
			xlhtml -a "$file" > "$tmp/page.html"
			elinks -dump "$tmp/page.html"
			rm -rf "$tmp"
		} || \
		xls2csv "$file" | sed -e 's/,,/, ,/g' | column -s, -t || \
		strings "$file"
		;;
	[Xx][Ll][Ss][Xx])
		xlsx2csv "$file" | sed -e 's/,,/, ,/g' | column -s, -t
		;;
	[Zz][Ii][Pp])
		7z l "$file" || unzip -v "$file"
		;;
	[Zz][Oo][Oo])
		file -b "$file" && zoo l "$file"
		;;
	[Zz][Pp][Aa][Qq])
		file -b "$file" && zpaq l "$file"
		;;
	[Zz][Ss][Tt])
		file -b "$file" && tar -tvf "$file" || zstdcat "$file"
		;;
	*)
		case `file -b --mime-type -L "$file"` in
			"application/x-executable"|"application/x-sharedlib"|"application/x-pie-executable")
				file -b "$file"
				ldd  "$file" | grep "not found"
				readelf -a "$file"
				nm -CD "$file"
				;;
			"application/zip")
				7z l "$file" || unzip -v "$file"
				;;
			"application/x-sqlite3")
				sqlite3 "$file" .dbinfo .dump
				;;
			"inode/directory")
				stat "$file"
				;;
			"application/x-dosexec")
				exiftool  "$file" || \
				wrestool --extract --raw --type=version "$file" |  strings -el
				readpe -A "$file"
				;;
		esac
		;;
esac
