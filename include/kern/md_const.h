#ifndef _KERN_MD_CONST_H_
#define _KERN_MD_CONST_H_

#define SWI_ENTRY_POINT	0x28
#define IRQ_ENTRY_POINT	0x38
#define TEXT_REG_BASE	0x218000

#define CPU_MODE_MASK	0x1f
#define CPU_MODE_USER	0x10
#define CPU_MODE_IRQ	0x12
#define CPU_MODE_SVC	0x13
#define CPU_MODE_SYS	0x1f

#endif // _KERN_MD_CONST_H_
