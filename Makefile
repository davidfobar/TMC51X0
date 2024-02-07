.PHONY: clean
clean:
	rm -rf .pio

firmware: clean
	pio run -e teensy40

.PHONY: upload
upload: clean
	pio run -e teensy40 --target upload --upload-port /dev/ttyACM0

.PHONY: monitor
monitor:
	pio device monitor --echo --eol=LF
