# 1. Aşama: Derleme (Build Stage)
FROM gcc:latest AS builder

WORKDIR /usr/src/app

COPY . .

# Derleme komutu aynı kalıyor
RUN g++ -o http_server examples/hello_world.cpp src/*.cpp -Iinclude -pthread

# 2. Aşama: Çalıştırma (Run Stage)
# Hatayı çözmek için: Derleme yaptığımız aynı dağıtımın hafif versiyonunu kullanıyoruz
FROM gcc:latest

WORKDIR /root/

# Sadece gerekli olan binary dosyasını kopyalıyoruz
COPY --from=builder /usr/src/app/http_server .

EXPOSE 8080

CMD ["./http_server"]