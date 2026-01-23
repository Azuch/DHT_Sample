# Dự án Driver DHT11 cho Zephyr RTOS

## Tổng quan
Dự án này tập trung vào việc phát triển driver cho cảm biến nhiệt độ và độ ẩm DHT11 trong môi trường Zephyr RTOS. Chúng tôi đang xây dựng module driver hoàn chỉnh bao gồm tệp ràng buộc cây thiết bị (device tree binding), mã nguồn driver và các tệp cấu hình cần thiết.

## Ngôn ngữ phản hồi
Tất cả các phản hồi và tài liệu liên quan đến dự án này nên được viết bằng tiếng Việt để phù hợp với nhóm phát triển.

## Tiến độ hiện tại
- Đã tạo cấu trúc thư mục cơ bản cho module
- Đã tạo tệp ràng buộc thiết bị `dts/bindings/input/azuch,dht11.yaml`
- Đang nghiên cứu cách sử dụng đúng các thuộc tính trong tệp ràng buộc, đặc biệt là `dio-gpios` thay vì `gpios` như thông thường
- Đã hiểu rằng DHT11 sử dụng giao tiếp GPIO đơn chứ không phải I2C hay SPI
- Đã học về sự khác biệt giữa `phandle` và cách sử dụng `dio-gpios` trong tệp ràng buộc

## Kiến thức quan trọng
- DHT11 sử dụng một chân GPIO duy nhất cho cả truyền và nhận dữ liệu theo giao thức đặc biệt
- Trong tệp ràng buộc, nên sử dụng `dio-gpios` thay vì `gpios` để phản ánh đúng bản chất giao tiếp đặc biệt của DHT
- Tệp ràng buộc nên kế thừa từ `sensor-device.yaml` vì DHT11 là cảm biến
- `phandle` là kiểu dữ liệu đặc biệt trong Device Tree dùng để tham chiếu đến nút khác trong cây thiết bị

## Milestone: Hiểu rõ giao thức DHT11 và tích hợp Zephyr
- Đã phân tích chi tiết giao thức giao tiếp với DHT11
- Hiểu cách thiết lập GPIO với `GPIO_ACTIVE_LOW` và `GPIO_PULL_UP` trong device tree
- Biết cách xử lý tín hiệu từ cảm biến: gửi tín hiệu bắt đầu, đọc phản hồi, xử lý 40 bit dữ liệu
- Nắm vững vai trò của các tệp tiêu đề Zephyr: `device.h`, `sensor.h`, `byteorder.h`, `util.h`
- Hiểu cách tính toán ngưỡng phân biệt bit 0 và 1 từ độ dài xung
- Biết cách xác minh dữ liệu bằng checksum
- Hiểu lý do khóa ngắt (IRQ lock) có thể được sử dụng để đảm bảo độ chính xác về thời gian

## Milestone: Cải tiến kiểm tra tổng kiểm và quản lý chân GPIO
- Hiểu rõ lý do sử dụng phép toán `& 0xFF` trong xác thực tổng kiểm (checksum)
- Biết cách xử lý overflow khi cộng các byte dữ liệu
- Hiểu tầm quan trọng của việc đặt chân GPIO ở trạng thái không hoạt động sau khi đọc mẫu
- Đảm bảo tính ổn định của bus và ngăn ngừa can thiệp giữa các lần đọc
- Chuẩn bị đúng trạng thái cho phiên giao tiếp tiếp theo

## Các bước tiếp theo
- Đã hoàn thiện tệp ràng buộc `azuch,dht11.yaml` theo tiêu chuẩn Zephyr
- Triển khai mã nguồn driver trong `drivers/dht11/dht11.c`
- Viết header cho driver trong `drivers/dht11/dht11.h`
- Tạo ví dụ ứng dụng sử dụng driver
- Kiểm thử driver với phần cứng thực tế

## Tệp ràng buộc hiện tại (azuch,dht11.yaml)
```yaml
# SPDX-License-Identifier: Apache-2.0

description: DHT11 Humidity and Temperature Sensor

compatible: "azuch,dht11"

include: [sensor-device.yaml]

properties:
  dio-gpios:
    type: phandle-array
    required: true
    description: |
      Pin on which sensor communication will be performed.
      
      Control and data are encoded by the duration of active low
      signals. A pull up resistor may be appropriate.

  label:
    type: string
    required: true
    description: Human readable string describing the device
```