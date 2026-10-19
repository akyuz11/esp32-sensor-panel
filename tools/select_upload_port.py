# PlatformIO extra_script: yukleme portunu kartin USB seri numarasindan bulur.
#
# Iki kart da ayni USB kimligiyle (303A:1001) gorundugu icin PlatformIO hangisinin
# hangisi oldugunu bilemez. Her env platformio.ini'de kendi kartinin seri
# numarasini "custom_usb_serial" ile verir; bu betik yukleme sirasinda o seri
# numarali portu secer. Kart takili degilse yukleme durur: baska bir karta
# yanlislikla yazilmaz. --upload-port elle verilirse ona dokunulmaz.

Import("env")  # noqa: F821  (SCons tarafindan saglanir)

from SCons.Script import COMMAND_LINE_TARGETS  # noqa: E402

# Karta yazan ya da karti silen tum hedefler: "erase" de dahil, yoksa
# "pio run -e <env> -t erase" o an takili olan BASKA karti silebilirdi.
UPLOAD_TARGETS = {"upload", "uploadfs", "uploadfsota", "erase", "erase_upload"}


def find_port(serial_no):
    from serial.tools import list_ports

    for port in list_ports.comports():
        if port.serial_number and port.serial_number.upper() == serial_no.upper():
            return port.device
    return None


serial_no = env.GetProjectOption("custom_usb_serial", "").strip()  # noqa: F821
wants_upload = bool(UPLOAD_TARGETS.intersection(COMMAND_LINE_TARGETS))

if serial_no and wants_upload and not env.subst("$UPLOAD_PORT"):  # noqa: F821
    port = find_port(serial_no)
    if port is None:
        print(
            "\n*** %s karti (USB seri no %s) takili degil; yukleme durduruldu.\n"
            "*** Yanlis karta yazmamak icin baska bir porta yuklenmez.\n"
            % (env["PIOENV"], serial_no)  # noqa: F821
        )
        env.Exit(1)  # noqa: F821
    print("Yukleme portu: %s (USB seri no %s)" % (port, serial_no))
    env.Replace(UPLOAD_PORT=port)  # noqa: F821
