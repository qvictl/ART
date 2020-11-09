/* -*- C++ -*-
 *
 *  This file is part of ART.
 *
 *  Copyright 2020 Alberto Griggio <alberto.griggio@gmail.com>
 *
 *  ART is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  ART is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with ART.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "imgiomanager.h"
#include "subprocess.h"
#include "utils.h"
#include "settings.h"
#include "imagefloat.h"
#include "image8.h"
#include "image16.h"
#include <iostream>
#include <glib/gstdio.h>
#include <unistd.h>

namespace rtengine {

extern const Settings *settings;

namespace {

ImageIOManager instance;

class S: public Glib::ustring {
public:
    explicit S(const Glib::ustring &s): Glib::ustring(s) {}
};

std::ostream &operator<<(std::ostream &out, const S &s)
{
    try {
        return out << std::string(s);
    } catch (Glib::ConvertError &e) {
        return out << s.raw();
    }
}

} // namespace


ImageIOManager *ImageIOManager::getInstance()
{
    return &instance;
}


void ImageIOManager::init(const Glib::ustring &dirname)
{
    loaders_.clear();

    if (!Glib::file_test(dirname, Glib::FILE_TEST_IS_DIR)) {
        return;
    }

    dir_ = dirname;

    try {
        Glib::Dir dir(dirname);
        std::vector<std::string> dirlist(dir.begin(), dir.end());
        std::sort(dirlist.begin(), dirlist.end());

        for (auto &filename : dirlist) {
            auto ext = getFileExtension(filename).lowercase();
            if (ext != "txt") {
                continue;
            }
            
            const Glib::ustring pth = Glib::build_filename(dirname, filename);

            if (!Glib::file_test(pth, Glib::FILE_TEST_IS_REGULAR)) {
                continue;
            }

            try {
                const Glib::ustring group = "ART ImageIO";
                Glib::KeyFile kf;
                if (!kf.load_from_file(pth)) {
                    continue;
                }

                Format fmt = FMT_TIFF_FLOAT;

                Glib::ustring cmd;
                if (kf.has_key(group, "ReadCommand")) {
                    cmd = kf.get_string(group, "ReadCommand");
                } else {
                    continue;
                }

                std::string ext;
                if (kf.has_key(group, "Extension")) {
                    ext = kf.get_string(group, "Extension").lowercase();
                } else {
                    continue;
                }

                loaders_[ext] = cmd;

                if (settings->verbose) {
                    std::cout << "Found loader for extension \"" << ext << "\": " << S(cmd) << std::endl;
                }

                if (kf.has_key(group, "WriteCommand")) {
                    cmd = kf.get_string(group, "WriteCommand");
                    savers_[ext] = cmd;
                    Glib::ustring lbl;
                    if (kf.has_key(group, "Label")) {
                        lbl = kf.get_string(group, "Label");
                    } else {
                        lbl = Glib::ustring(ext).uppercase();
                    }

                    savelbls_[ext] = lbl;
                    
                    if (settings->verbose) {
                        std::cout << "Found saver for extension \"" << ext << "\": " << S(cmd) << std::endl;
                    }
                }

                if (kf.has_key(group, "Format")) {
                    auto f = kf.get_string(group, "Format").lowercase();
                    if (f == "jpg") {
                        fmt = FMT_JPG;
                    } else if (f == "png") {
                        fmt = FMT_PNG;
                    } else if (f == "png16") {
                        fmt = FMT_PNG16;
                    } else if (f == "tiff") {
                        fmt = FMT_TIFF;
                    } else if (f == "float") {
                        fmt = FMT_TIFF_FLOAT;
                    }
                }
                fmts_[ext] = fmt;
            } catch (Glib::Exception &exc) {
                std::cout << "ERROR loading " << S(pth) << ": " << S(exc.what())
                          << std::endl;
            }
        }
    } catch (Glib::Exception &exc) {
        std::cout << "ERROR scanning " << S(dirname) << ": " << S(exc.what()) << std::endl;
    }

    if (settings->verbose) {
        std::cout << "Loaded " << loaders_.size() << " custom loaders"
                  << std::endl;
    }
}


Glib::ustring ImageIOManager::get_ext(Format f)
{
    switch (f) {
    case FMT_JPG: return ".jpg";
    case FMT_PNG: case FMT_PNG16: return ".png";
    default: return ".tif";
    }
}


bool ImageIOManager::load(const Glib::ustring &fileName, ProgressListener *plistener, ImageIO *&img, int maxw_hint, int maxh_hint)
{
    auto ext = std::string(getFileExtension(fileName).lowercase());
    auto it = loaders_.find(ext);
    if (it == loaders_.end()) {
        return false;
    }
    if (plistener) {
        plistener->setProgressStr("PROGRESSBAR_LOADING");
        plistener->setProgress(0.0);
    }

    std::string templ = Glib::build_filename(Glib::get_tmp_dir(), Glib::ustring::compose("ART-load-%1-XXXXXX", Glib::path_get_basename(fileName)));
    int fd = Glib::mkstemp(templ);
    if (fd < 0) {
        return false;
    }
    auto fmt = fmts_[ext];
    Glib::ustring outname = Glib::filename_to_utf8(templ) + get_ext(fmt);
    // int exit_status = -1;
    std::vector<Glib::ustring> argv = subprocess::split_command_line(it->second);
    argv.push_back(fileName);
    argv.push_back(outname);
    argv.push_back(std::to_string(maxw_hint));
    argv.push_back(std::to_string(maxh_hint));
    std::string sout, serr;
    bool ok = true;
    if (settings->verbose) {
        std::cout << "loading " << fileName << " with " << it->second << std::endl;
    }
    try {
        subprocess::exec_sync(dir_, argv, true, &sout, &serr);
    } catch (subprocess::error &err) {
        if (settings->verbose) {
            std::cout << "  exec error: " << err.what() << std::endl;
        }
        ok = false;
    }
    close(fd);
    g_remove(templ.c_str());
    if (settings->verbose) {
        if (!sout.empty()) {
            std::cout << "  stdout: " << sout << std::flush;
        }
        if (!serr.empty()) {
            std::cout << "  stderr: " << serr << std::flush;
        }
    }
    if (!ok) {
        if (Glib::file_test(outname, Glib::FILE_TEST_EXISTS)) {
            g_remove(outname.c_str());
        }
        return false;
    }

    IIOSampleFormat sFormat;
    IIOSampleArrangement sArrangement;
    bool err = false;

    switch (fmt) {
    case FMT_JPG:
        sFormat = IIOSF_UNSIGNED_CHAR;
        sArrangement = IIOSA_CHUNKY;
        break;
    case FMT_PNG:
    case FMT_PNG16:
        err = ImageIO::getPNGSampleFormat(outname, sFormat, sArrangement) != IMIO_SUCCESS;
        break;
    default:
        err = ImageIO::getTIFFSampleFormat(outname, sFormat, sArrangement) != IMIO_SUCCESS;
        break;
    }
        
    if (err) {
        if (Glib::file_test(outname, Glib::FILE_TEST_EXISTS)) {
            g_remove(outname.c_str());
        }
        return false;
    }

    bool ret = true;
    ImageIO *fimg = nullptr;
    
    switch (sFormat) {
    case IIOSF_UNSIGNED_CHAR:
        fimg = new Image8;
        break;
    case IIOSF_UNSIGNED_SHORT:
        fimg = new Image16;
        break;
    case IIOSF_LOGLUV24:
    case IIOSF_LOGLUV32:
    case IIOSF_FLOAT16:
    case IIOSF_FLOAT24:
    case IIOSF_FLOAT32:
        img = new Imagefloat;
        break;
    default:
        ret = false;
    }

    if (ret) {
        fimg->setProgressListener(plistener);
        fimg->setSampleFormat(sFormat);
        fimg->setSampleArrangement(sArrangement);

        if (fimg->load(outname)) {
            delete fimg;
            ret = false;
        } else {
            img = fimg;
        }} else {
        if (fimg) {
            delete fimg;
        }
    }

    if (Glib::file_test(outname, Glib::FILE_TEST_EXISTS)) {
        g_remove(outname.c_str());
    }
    return ret;
}


bool ImageIOManager::save(IImagefloat *img, const std::string &ext, const Glib::ustring &fileName, ProgressListener *plistener)
{
    auto it = savers_.find(ext);
    if (it == savers_.end()) {
        return false;
    }
    if (plistener) {
        plistener->setProgressStr("PROGRESSBAR_SAVING");
        plistener->setProgress(0.0);
    }

    std::string templ = Glib::build_filename(Glib::get_tmp_dir(), Glib::ustring::compose("ART-save-%1-XXXXXX", Glib::path_get_basename(fileName)));
    int fd = Glib::mkstemp(templ);
    if (fd < 0) {
        return false;
    }
    auto fmt = fmts_[ext];
    Glib::ustring tmpname = Glib::filename_to_utf8(templ) + get_ext(fmt);

    bool ok = false;

    switch (fmt) {
    case FMT_JPG:
        ok = (img->saveAsJPEG(tmpname) == 0);
        break;
    case FMT_PNG:
        ok = (img->saveAsPNG(tmpname, 8) == 0);
        break;
    case FMT_PNG16:
        ok = (img->saveAsPNG(tmpname, 16) == 0);
        break;
    case FMT_TIFF:
        ok = (img->saveAsTIFF(tmpname, 16, false, true) == 0);
        break;
    default:
        ok = (img->saveAsTIFF(tmpname, 32, true, true) == 0);
        break;
    }

    if (plistener) {
        plistener->setProgress(0.5);
    }
        
    if (ok) {
        std::vector<Glib::ustring> argv = subprocess::split_command_line(it->second);
        argv.push_back(tmpname);
        argv.push_back(fileName);
        std::string sout, serr;
        if (settings->verbose) {
            std::cout << "saving " << fileName << " with " << it->second << std::endl;
        }
        try {
            subprocess::exec_sync(dir_, argv, true, &sout, &serr);
        } catch (subprocess::error &err) {
            if (settings->verbose) {
                std::cout << "  exec error: " << err.what() << std::endl;
            }
            ok = false;
        }
        if (settings->verbose) {
            if (!sout.empty()) {
                std::cout << "  stdout: " << sout << std::flush;
            }
            if (!serr.empty()) {
                std::cout << "  stderr: " << serr << std::flush;
            }
        }
    }
    
    if (plistener) {
        plistener->setProgress(1.0);
    }

    close(fd);
    g_remove(templ.c_str());
    if (Glib::file_test(tmpname, Glib::FILE_TEST_EXISTS)) {
        g_remove(tmpname.c_str());
    }

    return ok;
}


std::vector<std::pair<std::string, Glib::ustring>> ImageIOManager::getSaveFormats() const
{
    std::vector<std::pair<std::string, Glib::ustring>> ret(savelbls_.begin(), savelbls_.end());
    return ret;
}

} // namespace rtengine
