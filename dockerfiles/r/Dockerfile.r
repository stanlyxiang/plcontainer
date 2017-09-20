FROM centos:7

EXPOSE 8080

RUN mkdir -p /opt && mkdir -p /opt/r && mkdir -p /opt/ds

ADD ./bin_r_*.tar.gz /opt/r/
ADD ./DataScienceR-*x86_64.rpm /opt/ds/
ADD ./install_r.sh /opt/
ADD ./r_env.sh /opt/

RUN /opt/install_r.sh

ENV R_HOME "/usr/lib64/R/lib64/R"
ENV PATH "/usr/lib64/R/bin:$PATH"
ENV LD_LIBRARY_PATH "/usr/lib64/R/lib64/R/lib:/usr/lib64/R/lib64/R/extlib:/opt/ds/extlib:$LD_LIBRARY_PATH"
