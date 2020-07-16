#include "wnd_main.h"
#include "ui_wnd_main.h"
#include <QMessageBox>

extern void (*g_event_func)(const char* which,const char* state);

template <typename base_t>
static inline const std::string to_hex(const base_t v) {
    std::stringstream stream;
    stream << std::hex << v;
    return stream.str();
}

wnd_main::wnd_main(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::wnd_main) {

    ui->setupUi(this);
    m_code_list_index = -1;
    this->init();
}

wnd_main::~wnd_main()
{
    delete ui;
}

bool wnd_main::export_binary(const std::string& path,const std::string& flags) {
    return true;
}

void wnd_main::show_msgbox(const std::string& text) {
    QMessageBox msgbox;
    msgbox.setText(text.c_str());
    msgbox.exec();
}

void wnd_main::add_list_item(const std::string& s) {
    ui->list_cpu_instructions->addItem(s.c_str());
}

void wnd_main::highlight_list_item(const size_t index) {
   // show_msgbox(std::to_string(index));
    if (!ui->chk_trace->isChecked())
        return;
    ui->list_cpu_instructions->setCurrentRow((int)index);
    //show_msgbox("OK");
}

void wnd_main::push_msg(const char* which,const char* data) {

}

void wnd_main::pass_data(const char* class_name,void* data,const uint32_t sz) {

    std::string tmp = std::string(class_name);
    if (tmp == "cpu_ctx") {
        shared_contexts_utils::deep_copy(&m_cpu_context,static_cast<const cpu_ctx_t*>(data));
        upd_cpu_ctx();
    } else if (tmp == "cpu_diss") { // XXX : WARNING : ASSUMES CPU CONTEXT IS FRESH! (AKA UPDATED BEFORE)
        shared_contexts_utils::deep_copy_diss(&m_cpu_diss,static_cast<const cpu_diss_t*>(data));
        auto it = m_code.find(m_cpu_diss.pc);
        if (it == m_code.end()) {
            std::string cc = m_cpu_diss.buf;
            size_t i = cc.find(';');
            if (i != std::string::npos) {
                cc = cc.substr(0,i);
            }
              add_list_item(std::string( to_hex(m_cpu_diss.pc) + ":" + cc ).c_str());
            m_code.insert({m_cpu_diss.pc, code_field_t ( cc  , m_cpu_context.pc , m_cpu_context.sp , m_cpu_context.pr , m_cpu_context.r,
                          m_cpu_context.fr , m_reverse_code_indices.size())});

             highlight_list_item( m_reverse_code_indices.size() );
            m_reverse_code_indices.push_back( m_cpu_diss.pc  );

        } else {
            std::string cc = m_cpu_diss.buf;
            size_t i = cc.find(';');
            if (i != std::string::npos) {
                cc = cc.substr(0,i);
            }
            if (it->second.diss != cc) {
                it->second.diss = cc;
                it->second.pc = m_cpu_context.pc;
                it->second.sp = m_cpu_context.sp;
                it->second.pr = m_cpu_context.pr;

                memcpy(it->second.gpr,m_cpu_context.r,sizeof(m_cpu_context.r));
                memcpy(it->second.fpr,m_cpu_context.fr,sizeof(m_cpu_context.fr));
            }
              highlight_list_item( it->second.list_index );
        }
    }
}

void wnd_main::upd_live_ctx(const std::string& s) {
    ui->lbl_reg->setText(s.c_str());
}

void wnd_main::upd_cpu_ctx() {
    std::string lbl_txt;

    lbl_txt = "PC:" + to_hex(m_cpu_context.pc) + "\n";
    lbl_txt += "PR:" + to_hex(m_cpu_context.pr) + "\n";
    lbl_txt += "SP:" + to_hex(m_cpu_context.sp) + "\n";

    for (size_t i = 0;i < 16;++i) {
        lbl_txt += "R" + std::to_string(i) + ":" + to_hex(m_cpu_context.r[i])  +"\n";
    }

    for (size_t i = 0;i < 16;++i) {
        lbl_txt +=  "fR" + std::to_string(i) + ":" + std::to_string(m_cpu_context.fr[i])  ;
        lbl_txt +=  " | fR" + std::to_string(16+i) + ":" + std::to_string(m_cpu_context.fr[16+i])  +"\n";
    }
    emit upd_live_ctx(lbl_txt.c_str());
}

void wnd_main::init() {
    std::string lbl_txt;

    ui->reg_combo_box->addItem("PC");
    ui->reg_combo_box->addItem("SP");
    ui->reg_combo_box->addItem("PR");

    lbl_txt = "PC:$0\nSP:$0\nPR:$0\n";

    for (size_t i = 0;i < 16;++i) {
        lbl_txt += "R" + std::to_string(i) + ":$0\n";
        ui->reg_combo_box->addItem(std::string("R" + std::to_string(i)).c_str());
    }

    for (size_t i = 0;i < 16;++i) {
        lbl_txt +=  "fR" + std::to_string(i)  +":$0\n";
        ui->reg_combo_box->addItem(std::string("fR" + std::to_string(i)).c_str());
    }

    ui->lbl_reg->setText(lbl_txt.c_str());
}

void wnd_main::on_pushButton_7_clicked()
{
    g_event_func("set_pc",ui->txt_set_pc->text().toStdString().c_str());
}

void wnd_main::on_pushButton_6_clicked()
{
    uint32_t data[2]; //todo : FPR / SP / PC / ETC

    g_event_func("set_reg",(const char*)data);
}


void wnd_main::on_list_cpu_instructions_itemDoubleClicked(QListWidgetItem *item)
{

}

void wnd_main::on_list_cpu_instructions_itemClicked(QListWidgetItem *item)
{
   int32_t i = ui->list_cpu_instructions->row(item);

   if (i < 0) {
       m_code_list_index = -1;
       return;
   }
   else if ((size_t)i >= m_reverse_code_indices.size()) {
       m_code_list_index = -1;
       return;
   }

   uint32_t pc = m_reverse_code_indices[i];
   const code_field_t& fld = m_code[pc];

   std::string lbl_txt;

   m_code_list_index = i;
   lbl_txt = "PC:" + to_hex(fld.pc) + "\n";
   lbl_txt += "PR:" + to_hex(fld.pr) + "\n";
   lbl_txt += "SP:" + to_hex(fld.sp) + "\n";

   for (size_t i = 0;i < 16;++i) {
       lbl_txt += "R" + std::to_string(i) + ":" + to_hex(fld.gpr[i])  +"\n";
   }
   for (size_t i = 0;i < 16;++i) { //32 FpRs
       lbl_txt +=  "fR" + std::to_string(i) + ":" + std::to_string(fld.fpr[i])  ;
       lbl_txt +=  " | fR" + std::to_string(16+i) + ":" + std::to_string(fld.fpr[16+i])  +"\n";
   }
   ui->lbl_state_data->setText(lbl_txt.c_str());
}

void wnd_main::on_pushButton_clicked()
{
    g_event_func("step","");
}

void wnd_main::on_pushButton_11_clicked()
{
    g_event_func("stop","");
}

void wnd_main::on_pushButton_12_clicked()
{
    g_event_func("start","");
}

void wnd_main::on_pushButton_13_clicked()
{
    g_event_func("reset","");
}

void wnd_main::on_chk_active_toggled(bool checked)
{
    g_event_func("dbg_enable",(checked) ? "true":"false");
}
