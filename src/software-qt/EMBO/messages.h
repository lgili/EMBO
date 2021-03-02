/*
 * CTU/EMBO - EMBedded Oscilloscope <github.com/parezj/EMBO>
 * Author: Jakub Parez <parez.jakub@gmail.com>
 */

#ifndef MESSAGES_H
#define MESSAGES_H

#include "msg.h"

#include <QObject>

#define EMBO_IDN            "*IDN"
#define EMBO_RST            "*RST"

#define EMBO_SYS_LIMS       ":SYS:LIM"
#define EMBO_SYS_INFO       ":SYS:INFO"
#define EMBO_SYS_MODE       ":SYS:MODE"

#define EMBO_VM_READ        ":VM:READ"

#define EMBO_SCOP_READ      ":SCOP:READ"
#define EMBO_SCOP_SET       ":SCOP:SET"
#define EMBO_SCOP_FORCETRIG ":SCOP:FORC"
#define EMBO_SCOP_AVERAGE   ":SCOP:AVER"

#define EMBO_LA_READ        ":LA:READ"
#define EMBO_LA_SET         ":LA:SET"
#define EMBO_LA_FORCETRIG   ":LA:FORC"

#define EMBO_CNTR_ENABLE    ":CNTR:ENA"
#define EMBO_CNTR_READ      ":CNTR:READ"

#define EMBO_SGEN_SET       ":SGEN:SET"

#define EMBO_PWM_SET        ":PWM:SET"

#define EMBO_SET_TRUE       "1"
#define EMBO_SET_FALSE      "0"

/***************************** Messages - SYS *****************************/

class Msg_Idn : public Msg
{
    Q_OBJECT
public:
    explicit Msg_Idn(QObject* parent=0) : Msg(EMBO_IDN, true, parent) {};
    virtual void on_dataRx() override;
};

class Msg_Rst : public Msg
{
    Q_OBJECT
public:
    explicit Msg_Rst(QObject* parent=0) : Msg(EMBO_RST, false, parent) {};
    virtual void on_dataRx() override;
};

class Msg_SYS_Lims : public Msg
{
    Q_OBJECT
public:
    explicit Msg_SYS_Lims(QObject* parent=0) : Msg(EMBO_SYS_LIMS, true, parent) {};
    virtual void on_dataRx() override;
};

class Msg_SYS_Info : public Msg
{
    Q_OBJECT
public:
    explicit Msg_SYS_Info(QObject* parent=0) : Msg(EMBO_SYS_INFO, true, parent) {};
    virtual void on_dataRx() override;
};

class Msg_SYS_Mode : public Msg
{
    Q_OBJECT
public:
    explicit Msg_SYS_Mode(QObject* parent=0) : Msg(EMBO_SYS_MODE, true, parent) {};
    virtual void on_dataRx() override;
};

/***************************** Messages - VM ****************************/

class Msg_VM_Read : public Msg
{
    Q_OBJECT
public:
    explicit Msg_VM_Read(QObject* parent=0) : Msg(EMBO_VM_READ, true, parent) {};
    virtual void on_dataRx() override;
};

/***************************** Messages - SCOP **************************/

class Msg_SCOP_Read : public Msg
{
    Q_OBJECT
public:
    explicit Msg_SCOP_Read(QObject* parent=0) : Msg(EMBO_SCOP_READ, true, parent) {};
    virtual void on_dataRx() override;
};

class Msg_SCOP_Set : public Msg
{
    Q_OBJECT
public:
    explicit Msg_SCOP_Set(QObject* parent=0) : Msg(EMBO_SCOP_SET, true, parent) {};
    virtual void on_dataRx() override;
};

class Msg_SCOP_ForceTrig : public Msg
{
    Q_OBJECT
public:
    explicit Msg_SCOP_ForceTrig(QObject* parent=0) : Msg(EMBO_SCOP_FORCETRIG, false, parent) {};
    virtual void on_dataRx() override;
};

class Msg_SCOP_Average : public Msg
{
    Q_OBJECT
public:
    explicit Msg_SCOP_Average(QObject* parent=0) : Msg(EMBO_SCOP_AVERAGE, false, parent) {};
    virtual void on_dataRx() override;
};

/***************************** Messages - LA ****************************/

class Msg_LA_Read : public Msg
{
    Q_OBJECT
public:
    explicit Msg_LA_Read(QObject* parent=0) : Msg(EMBO_LA_READ, true, parent) {};
    virtual void on_dataRx() override;
};

class Msg_LA_Set : public Msg
{
    Q_OBJECT
public:
    explicit Msg_LA_Set(QObject* parent=0) : Msg(EMBO_LA_SET, true, parent) {};
    virtual void on_dataRx() override;
};

class Msg_LA_ForceTrig : public Msg
{
    Q_OBJECT
public:
    explicit Msg_LA_ForceTrig(QObject* parent=0) : Msg(EMBO_LA_FORCETRIG, false, parent) {};
    virtual void on_dataRx() override;
};

/***************************** Messages - CNTR **************************/

class Msg_CNTR_Enable : public Msg
{
    Q_OBJECT
public:
    explicit Msg_CNTR_Enable(QObject* parent=0) : Msg(EMBO_CNTR_ENABLE, true, parent) {};
    virtual void on_dataRx() override;
signals:
    void result(bool isQuery, bool enable);
};

class Msg_CNTR_Read : public Msg
{
    Q_OBJECT
public:
    explicit Msg_CNTR_Read(QObject* parent=0) : Msg(EMBO_CNTR_READ, true, parent) {};
    virtual void on_dataRx() override;
signals:
    void result(QString freq, QString period);
};

/***************************** Messages - SGEN **************************/

class Msg_SGEN_Set : public Msg
{
    Q_OBJECT
public:
    explicit Msg_SGEN_Set(QObject* parent=0) : Msg(EMBO_SGEN_SET, true, parent) {};
    virtual void on_dataRx() override;
};

/***************************** Messages - PWM ***************************/

class Msg_PWM_Set : public Msg
{
    Q_OBJECT
public:
    explicit Msg_PWM_Set(QObject* parent=0) : Msg(EMBO_PWM_SET, true, parent) {};
    virtual void on_dataRx() override;
};

#endif // MESSAGES_H