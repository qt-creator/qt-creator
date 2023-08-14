ARG PWDMODE=with


FROM ubuntu:20.04 AS base
ARG DEBIAN_FRONTEND=noninteractive
ENV TZ=Etc/UTC
RUN apt-get update && apt-get install -y squid apache2-utils && rm -rf /var/lib/apt/lists/*
COPY run.sh /
RUN chmod +x /run.sh


FROM base as image-with-pwd
RUN echo 1234 | htpasswd -i -c /etc/squid/pswds user
COPY userauth.conf /etc/squid/conf.d/

FROM base as image-without-pwd
COPY noauth.conf /etc/squid/conf.d/

FROM image-${PWDMODE}-pwd AS final
CMD [ "/run.sh" ]
