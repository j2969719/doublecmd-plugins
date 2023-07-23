all:

dist:
	$(foreach file, $(wildcard *.lua), \
		echo -e "[plugininstall]\ndescription=`grep wdx/scripts/$(file) ../../../plugins.md -1 | tail -1`\ntype=wdx\nfile=$(file)\ndefaultdir=scripts" > pluginst.inf && \
		tar -h -cvzf '../../dist/wdx_script_$(basename $(file))_$(shell date +%y.%m.%d).tar.gz' pluginst.inf $(wildcard $(basename $(file)).*) && \
		rm pluginst.inf || echo turboencabulator ; \
	)

	$(foreach folder, $(wildcard */), \
		echo $(patsubst %/,%,$(folder)) ; \
		echo -e "[plugininstall]\ndescription=`grep wdx/scripts/$(patsubst %/,%,$(folder)) ../../../plugins.md -1 | tail -1`\ntype=wdx\nfile=$(notdir $(wildcard $(folder)*wdx.lua))\ndefaultdir=scripts" > $(folder)pluginst.inf && \
		tar -C $(folder) -h -cvzf '../../dist/wdx_script_$(basename $(notdir $(wildcard $(folder)*wdx.lua)))_$(shell date +%y.%m.%d).tar.gz' pluginst.inf $(notdir $(wildcard $(folder)*)) && \
		rm $(folder)pluginst.inf || echo turboencabulator ; \
	)

