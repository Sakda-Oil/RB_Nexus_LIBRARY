# คู่มือย่อ: แก้โค้ด เปลี่ยนเวอร์ชัน และอัปเดต RB_Nexus

ปรับปรุง 7 ตุลาคม 2026 · อ้างอิงซอร์ส **0.2.1** · Windows PowerShell

**0.2.2 ในเอกสารนี้เป็นตัวอย่างรุ่นถัดไป ไม่ใช่รุ่นที่เผยแพร่แล้ว**

อ่านบทสอนพร้อมคำอธิบายและวิธีแก้ปัญหาใน [คู่มือแก้โค้ด HTML](html/how_to_modify.html) และ [คู่มือออก Release HTML](html/release.html) ดาวน์โหลด repository แล้วเปิดไฟล์ HTML ในเบราว์เซอร์ได้ มีปุ่มพิมพ์/บันทึกเป็น PDF

## 1. เตรียมงาน

เปิด PowerShell ที่โฟลเดอร์โปรเจกต์ ตรวจว่ามีงานค้างหรือไม่ก่อนเปลี่ยน branch หากคำสั่งใดผิดพลาด ให้แก้ก่อนทำขั้นถัดไป

```powershell
Set-Location 'D:\Desktop\Redbrick\RB_Nexus'
git status --short
# ถ้ามีรายการ ให้ตรวจและบันทึกงานเดิมก่อน
git switch main
git pull --ff-only origin main
git switch -c update/0.2.2
```

เลือกชื่อ branch ใหม่หากชื่อนี้มีอยู่แล้ว เครื่องใหม่ต้องติดตั้ง Git, Python 3, Arduino CLI และ dependencies ตาม [บทเตรียมเครื่อง](html/how_to_modify.html#tools) เพราะ `.cache` ไม่มากับการ clone

## 2. เลือกไฟล์ที่ต้องแก้

| ต้องการแก้ | ไฟล์ |
|---|---|
| เลือก MPU6050 / MPU9250 / BNO085 | `libraries/RB_Nexus/examples/IMURaw/IMURaw.ino` หรือ sketch ส่วนตัว |
| เพิ่ม API | `libraries/RB_Nexus/src/RB_Nexus.h` และ implementation ใน `.cpp` |
| ไดรเวอร์ IMU | `libraries/RB_Nexus/src/RB_Nexus_IMU.cpp` |
| มอเตอร์ / PID / Servo / CAN | `libraries/RB_Nexus/src/RB_Nexus.cpp` |
| micro-ROS | `libraries/RB_Nexus/src/RB_Nexus_MicroROS.h` และ `libraries/RB_Nexus/examples/micro_ros/` |
| คู่มือ | `docs/html/` และ `README.md` |
| ทดสอบพฤติกรรม | `tests/test_driver.cpp`, `tests/test_imu.cpp` |

ตัวอย่างการเลือก IMU: ใช้ `RB.imuBegin(RBIMUType::MPU6050, 0x68)` หรือเปลี่ยนชนิดเป็น `MPU9250` ที่ `0x68` หรือ `BNO085` ที่ `0x4A` ตามโมดูลจริง คง `RB.begin()`, `RB.update()` และการตรวจ `imuDataFresh()` ไว้ อ่านข้อจำกัด/การต่อสายใน [คู่มือ IMU](html/imu.html)

ถ้าแก้ตัวอย่าง IMURaw ใน repository ต้องแก้ code block แรกของ README ให้ตรงด้วย แก้ซอร์สใน repository ไม่ใช่เฉพาะสำเนาใน Arduino15 ซึ่งอาจถูกแทนที่เมื่ออัปเดต

## 3. เปลี่ยนเลขรุ่นให้ตรงกัน 3 จุด

สำหรับการออกรุ่นซอฟต์แวร์ใหม่ ตัวอย่าง `0.2.1 → 0.2.2`:

| ไฟล์ | ค่าที่ต้องแก้ |
|---|---|
| `VERSION` | `0.2.2` เพียงบรรทัดเดียว ไม่มี `v` |
| `libraries/RB_Nexus/library.properties` | `version=0.2.2` |
| `libraries/RB_Nexus/src/RB_Nexus.h` | ค่า `RBNexus::version` เป็น `"0.2.2"` |

แก้ `docs/RELEASE_NOTES.md`, ข้อมูลรุ่นปัจจุบันใน README/HTML และเพิ่มรายงานผลทดสอบรุ่นใหม่ตามผลที่รันจริง เก็บรายงานรุ่นเก่าไว้ อย่าแทนที่เลขรุ่นทั้ง repository และอย่าแก้ root `package_RB_Nexus_index.json` หรือสร้าง tag ล่วงหน้า ระบบ Release ทำให้หลังตรวจผ่าน

## 4. ทดสอบก่อนส่งขึ้น GitHub

คำสั่งต่อไปนี้ใช้เครื่องมือที่เตรียมไว้ในเครื่องปัจจุบัน:

```powershell
$rbCli = (Resolve-Path '.cache/tools/arduino-cli.exe').Path
$rbConfig = (Resolve-Path '.cache/arduino-cli.yaml').Path
python -X utf8 scripts/check_docs.py
python -X utf8 scripts/test_host.py --zig .cache/tools/zig-windows-x86_64-0.13.0/zig.exe
python -X utf8 scripts/compile_examples.py --cli $rbCli --config-file $rbConfig
python -X utf8 scripts/build_package.py --repository Sakda-Oil/RB_Nexus_LIBRARY
python -X utf8 scripts/verify_package.py
git diff --check
```

หยุดตรวจ error หากคำสั่งใดไม่ผ่าน ไม่ถือว่าคำสั่งท้ายสุดผ่านแล้วคำสั่งก่อนหน้าจะผ่านทั้งหมด ทดลองติดตั้ง archive ในเครื่องตาม [ขั้นตอนทดสอบแพ็กเกจ](html/release.html#local-install) แล้วคอมไพล์ด้วย `--installed-library` เพื่อยืนยันซอร์สที่บรรจุจริง ทดสอบฮาร์ดแวร์ส่วนที่แก้และบันทึกผลแยกจากผลคอมไพล์

## 5. ส่ง branch และออก Release

```powershell
git diff
git add libraries docs scripts tests README.md VERSION
# ถ้าแก้ไฟล์อื่น ให้เพิ่มชื่อไฟล์นั้นอย่างเจาะจง
git diff --cached --check
git diff --cached
git commit -m "fix: describe changes for 0.2.2"
git push -u origin update/0.2.2
```

1. เปิด pull request ของ branch นี้เข้า `main` บน GitHub
2. รอ **CI Verification** ผ่าน ตรวจ diff แล้ว Merge
3. เปิด [Actions](https://github.com/Sakda-Oil/RB_Nexus_LIBRARY/actions) รอ **CI Verification** และ **Build and release** สำเร็จ
4. เปิด [Releases](https://github.com/Sakda-Oil/RB_Nexus_LIBRARY/releases) ตรวจไฟล์แนบ `RB_Nexus-esp32-0.2.2.zip` และ `package_RB_Nexus_index.json`
5. ตรวจว่า root index อัปเดตโดย bot และทดลองติดตั้งผ่าน Boards Manager ด้วย URL เดิม
6. กลับเครื่อง สั่ง `git switch main` แล้ว `git pull --ff-only origin main` เพื่อรับ index ล่าสุด

**workflow ปัจจุบันออก Release เมื่อ push เข้า main** จึงต้องทำงานบน branch ก่อนและเตรียมเลขรุ่นใหม่ก่อน Merge ไม่ใช่แค่ push tag และการ push feature branch อย่างเดียวไม่ได้เรียก CI จนกว่าจะเปิด PR

ถ้า Release ล้มเหลว อ่าน error แรกใน Actions อย่า force push หรือเขียนทับ archive ที่เผยแพร่แล้ว แก้ข้อผิดพลาดที่พบหลังเผยแพร่เป็นรุ่นถัดไป

## 6. แก้เฉพาะคู่มือ โดยไม่ออกรุ่นซอฟต์แวร์

คงเลขรุ่นทั้ง 3 จุดไว้ ตรวจลิงก์และเปิดอ่าน HTML แล้ว commit เฉพาะเอกสาร ใช้ `[skip ci]` ในข้อความ commit สำหรับ workflow ปัจจุบันเพื่อไม่ออก Release เลขเดิม:

```powershell
python -X utf8 scripts/check_docs.py
git diff --check
git add docs/html/how_to_modify.html docs/html/release.html
# เพิ่มเอกสารอื่นเฉพาะที่แก้จริง แล้วตรวจรายการก่อน commit
git diff --cached
git commit -m "docs: improve maintenance guide [skip ci]"
git push origin main
```

ตัวอย่างนี้ใช้เมื่ออยู่บน `main` ที่ sync ล่าสุดและ repository อนุญาต direct push เท่านั้น ถ้านโยบายบังคับ PR/required checks ให้ทำตามนโยบายและปรับ release trigger ก่อน เพราะ skipped checks อาจค้าง Pending ห้ามใช้ skip กับงานแก้โค้ด ดู [ขั้นตอนเอกสารอย่างเดียว](html/release.html#docs-only) และ [คำอธิบายจาก GitHub](https://docs.github.com/en/actions/how-tos/manage-workflow-runs/skip-workflow-runs)

## เช็กลิสต์ก่อนเผยแพร่

- [ ] เลขรุ่นใหม่ตรงกันทั้ง 3 จุดและไม่ซ้ำรุ่นที่เผยแพร่แล้ว
- [ ] API, ตัวอย่าง, README และ HTML ตรงกัน
- [ ] ผลทดสอบผ่านและระบุสิ่งที่ยังไม่ได้ตรวจบนบอร์ดอย่างชัดเจน
- [ ] archive ผ่านการตรวจและทดลองติดตั้ง
- [ ] ไม่มีรหัสผ่านหรือ token จริงในไฟล์ที่ส่งขึ้น GitHub
- [ ] CI ผ่านก่อน Merge และ Release workflow ผ่านหลัง Merge
- [ ] assets และ root index ดาวน์โหลดได้ และ Boards Manager ติดตั้งรุ่นใหม่ได้
