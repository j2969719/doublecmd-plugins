#!/bin/bash

#(EXT="ISO")|(EXT="TORRENT")|(EXT="SO")|(EXT="MO")|(EXT="DEB")|(EXT="TAR")|(EXT="LHA")|(EXT="ARJ")|(EXT="CAB")|(EXT="HA")|(EXT="RAR")|(EXT="ALZ")|(EXT="CPIO")|(EXT="7Z")|(EXT="ACE")|(EXT="ARC")|(EXT="ZIP")|(EXT="ZOO")|(EXT="PS")|(EXT="PDF")|(EXT="ODT")|(EXT="DOC")|(EXT="XLS")|(EXT="DVI")|(EXT="DJVU")|(EXT="EPUB")|(EXT="HTML")|(EXT="HTM")|(EXT="EXE")|(EXT="DLL")|(EXT="GZ")|(EXT="BZ2")|(EXT="XZ")|(EXT="MSI")|(EXT="RTF")|(EXT="FB2")

file=$1
filetype="${1##*.}"
#echo "$filetype"
file -b "$1"
#du -h –max-depth=1 "$1"
case "${filetype}" in
	[Tt][Oo][Rr][Rr][Ee][Nn][Tt])
		ctorrent -x "$file" || transmission-show "$file"
		;;
	[Ss][Oo])
		file "$file" && nm -C -D "$file"
		;;
	[Mm][Oo])
		msgunfmt "$file" || cat "$file"
		;;
	[Dd][Ee][Bb])
		dpkg-deb -I "$file" && echo && dpkg-deb -c "$file"
		;;
	[Tt][Aa][Rr])
		tar tvvf - < "${file}"
		;;
	[Ll][Hh][Aa])
		lha l "$file"
		;;
	[Aa][Rr][Jj])
		arj l "$file" 2>/dev/null || \
			unarj l "$file"
		;;
	[Cc][Aa][Bb])
		cabextract -l "$file"
		;;
	[Hh][Aa])
		ha lf "$file"
		;;
	[Rr][Aa][Rr])
		rar v -c- "$file" 2>/dev/null || \
			unrar v -c- "$file"
		;;
	[Aa][Ll][Zz])
		unalz -l "$file"
		;;
	[Aa][Rr][Cc])
		arc l "$file"
		;;
	[Cc][Pp][Ii][Oo])
		cpio -itv < "$file" 2>/dev/null
		;;
	7[Zz]|[Gg][Zz]|[Xx][Zz]|[Tt][Xx][Zz]|[Ii][Mm][Gg]|[Ii][Mm][Aa]|[Bb][Zz]2)
		7za l "$file" 2>/dev/null || 7z l "$file"
		;;
	[Aa][Cc][Ee])
		unace v -y "$file"
		;;
	[Aa][Rr][Cc])
		arc l "$file"
		;;
	[Zz][Ii][Pp])
		7z l "$file" || unzip -v "$file"
		;;
	[Zz][Oo][Oo])
		zoo l "$file"
		;;
	[Mm][Ss][Ii])
		msiinfo suminfo "$file" && msiextract -l "$file"
		;;
	[Pp][Ss])
		ps2ascii "$file"
		;;
	[Pp][Dd][Ff])
		pdftotext -layout -nopgbrk "$file" -
		;;
	[Oo][Dd][Tt])
			odt2txt "$file"
		;;
	[Dd][Oo][Cc]|[Rr][Tt][Ff])
		catdoc -w "$file" #|  iconv -f "KOI8-R" -t "UTF-8"
		;;
	[Xx][Ll][Ss])
		which xlhtml >/dev/null 2>&1 && {
			tmp=`mktemp -d ${TMPDIR:-/tmp}/%p.XXXXXX`
			xlhtml -a "$file" > "$tmp/page.html"
			elinks -dump "$tmp/page.html"
			rm -rf "$tmp"
		} #|| \
		#	xls2csv "$file" |  iconv -f "KOI8R" -t "UTF-8" || \
		#	strings "$file"
		;;
	[Xx][Ll][Ss][Xx])
		xlsx2csv "$file" | sed -e 's/,,/, ,/g' | column -s, -t
		;;
	[Dd][Vv][Ii])
		which dvi2tty >/dev/null 2>&1 && \
			dvi2tty "$file" || \
			catdvi "$file"
		;;
	[Dd][Jj][Vv][Uu]|[Dd][Jj][Vv])
		djvused -e print-pure-txt "$file"
		;;
	[Cc][Ss][Vv])
		cat "$file" | sed -e 's/,,/, ,/g' | column -s, -t
		;;
	[Ee][Pp][Uu][Bb])
		einfo -v "$file"
		;;
	[Aa][Nn][Ss])
		strings | cat "$file" |  iconv -f "866" -t "UTF-8"
		;;
	[Zz][Pp][Aa][Qq])
		zpaq l "$file"
		;;
	[Dd][Oo][Cc][Xx])
		#docx2txt.pl "$file" -
		unzip -p "$file" | grep --text '<w:r' | sed 's/<w:p[^<\/]*>/ \
			/g' | sed 's/<[^<]*>//g' | grep -v '^[[:space:]]*$' | sed G
		;;
	[Ee][Xx][Ee]|[Dd][Ll][Ll])
		exiftool "$file" || \
		wrestool --extract --raw --type=version "$file" |  strings -el
		;;
	[Ff][Bb][2])
		xsltproc $COMMANDER_PATH/scripts/FB2_2_txt_ru.xsl "$file" |  iconv -f "windows-1251" -t "UTF-8"
		;;
	[Ii][Ss][Oo])
		isoinfo -d -i "$file" && isoinfo -l -R -J -i "$file"
		;;
	[Hh][Tt][Mm][Ll]|[Hh][Tt][Mm])
		links -dump "$file" 2>/dev/null || w3m -dump "$file" 2>/dev/null || lynx -dump "$file"
		;;
	*)
		case "$(file -b --mime-type $file)" in
			"application/x-executable")
				file "$file" && nm -C -D "$file" && ldd  "$file"
				;;
			"application/x-sharedlib")
				file "$file" && nm -C -D "$file" && ldd  "$file"
				;;
			"application/zip")
				7z l "$file" || unzip -v "$file"
				;;
			"application/x-sqlite3")
				sqlite3 "$file" .dbinfo .dump
				;;
			#"inode/directory")
				#cd "$file" && du -h --apparent-size | sort -hr
				#;;
		esac
		;;
esac
