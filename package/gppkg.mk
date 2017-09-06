PGXS := $(shell pg_config --pgxs)
include $(PGXS)

GP_VERSION_NUM := $(GP_MAJORVERSION)

MAJOR_OS=$(shell cat /etc/redhat-release | sed s/.*release\ // | sed s/\ .*// | awk -F '.' '{print $$1}' )
ARCH=$(shell uname -p)

RPM_ARGS=$(subst -, ,$*)
RPM_NAME=$(word 1,$(RPM_ARGS))
PWD=$(shell pwd)
%.rpm: 
	rm -rf RPMS BUILD SPECS
	mkdir RPMS BUILD SPECS
	cp $(RPM_NAME).spec SPECS/
	rpmbuild -bb SPECS/$(RPM_NAME).spec --buildroot $(PWD)/BUILD --define '_topdir $(PWD)' --define '__os_install_post \%{nil}' --define 'buildarch $(ARCH)' $(RPM_FLAGS)
	mv RPMS/$(ARCH)/$*.rpm .
	rm -rf RPMS BUILD SPECS

gppkg_spec.yml: gppkg_spec.yml.in
	cat $< | sed "s/#arch/$(ARCH)/g" | sed "s/#os/rhel$(MAJOR_OS)/g" | sed 's/#gpver/$(GP_VERSION_NUM)/g' | sed "s/#plcver/$(PLC_GPPKG_VER)/g"> $@

%.gppkg: gppkg_spec.yml $(MAIN_RPM) $(DEPENDENT_RPMS)
	mkdir -p gppkg/deps 
	cp gppkg_spec.yml gppkg/
	cp $(MAIN_RPM) gppkg/ 
ifdef DEPENDENT_RPMS
	for dep_rpm in $(DEPENDENT_RPMS); do \
		cp $${dep_rpm} gppkg/deps; \
	done
endif
	source $(GPHOME)/greenplum_path.sh && gppkg --build gppkg 
	rm -rf gppkg

clean:
	rm -rf RPMS BUILD SPECS
	rm -rf gppkg
	rm -f gppkg_spec.yml
ifdef EXTRA_CLEAN
	rm -f $(EXTRA_CLEAN)
endif

install: $(TARGET_GPPKG)
	source $(INSTLOC)/greenplum_path.sh && gppkg -i $(TARGET_GPPKG)

.PHONY: install clean
