/* Memory Space Definitions */
MEMORY
{
  blrom    (rx)  : ORIGIN = 0x00000000, LENGTH = 0x00002000
  approm   (rx)  : ORIGIN = 0x00002000, LENGTH = 0x0003D000
  rom2     (rx)  : ORIGIN = 0x0003F000, LENGTH = 0x0001000
  rom3     (rw)  : ORIGIN = 0x00804000, LENGTH = 0x00000100
  ram      (rwx) : ORIGIN = 0x20000000, LENGTH = 0x00008000
}